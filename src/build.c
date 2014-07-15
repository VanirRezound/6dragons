/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Online building and editing module                                    *
 ***************************************************************************/

#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "h/mud.h"
#include "h/hometowns.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/shops.h"
#include "h/languages.h"
#include "h/city.h"

bool                    is_head_architect( CHAR_DATA *ch );
bool                    vnums_are_free( const char *check, int low_range, int high_range );
char                   *sprint_reset( RESET_DATA *pReset, short *num );
void                    fix_exits( void );
void                    string_stripcolor( char *argument, char *string );

int                     generate_hp( int level, int num, int size, int plus );
int                     get_containerflag( char *flag );
extern int              top_affect;
extern int              top_reset;
extern int              top_ed;
extern bool             fBootDb;
REL_DATA               *first_relation = NULL;
REL_DATA               *last_relation = NULL;

int get_currency_type   args( ( char *type ) );
void                    assign_currindex( ROOM_INDEX_DATA *room );

/*
 * Exit Pull/push types
 * (water, air, earth, fire)
 */
const char             *const ex_pmisc[] =
    { "undefined", "vortex", "vacuum", "slip", "ice", "mysterious" };

const char             *const ex_pwater[] = { "current", "wave", "whirlpool", "geyser" };

const char             *const ex_pair[] = { "wind", "storm", "coldwind", "breeze" };

const char             *const ex_pearth[] = { "landslide", "sinkhole", "quicksand", "earthquake" };

const char             *const ex_pfire[] = { "lava", "hotair" };

const char             *const ex_flags[] = {
    "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
    "dig", "eatkey", "nopassdoor", "hidden", "passage", "portal", "r1", "r2",
    "can_climb", "can_enter", "can_leave", "auto", "noflee", "searchable",
    "bashed", "bashproof", "nomob", "window", "can_look", "isbolt", "bolted",
    "autofull"
};

const char             *const container_flags[] = {
    "closeable", "pickproof", "closed", "locked", "eatkey", "r1", "r2", "r3",
    "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26",
    "r27"
};

const char             *const sec_flags[] = {
    "inside", "road", "field", "forest", "hills",
    "mountain", "water_swim", "water_noswim", "underwater", "air",
    "desert", "area_ent", "oceanfloor", "underground", "lava",
    "swamp", "city", "vroad", "hroad", "ocean",
    "jungle", "grassland", "crossroad", "thickforest", "highmountain",
    "arctic", "waterfall", "river", "dock", "lake",
    "campsite", "portalstone", "deepmud", "quicksand", "pastureland",
    "valley", "mountainpass", "beach", "fog", "nochange",
    "sky", "cloud", "dcloud", "ore", "max"
};

const char             *const r_flags[] = {
    "dark", "death", "nomob", "indoors", "petstore", "nomissile",
    "bv06", "nomagic", "tunnel", "private", "safe", "solitary",
    "petshop", "norecall", "tradeskills", "nodropall", "silence", "logspeech",
    "nodrop", "clanstoreroom", "nosummon", "noastral", "teleport", "teleshowdesc",
    "nofloor", "nosupplicate", "arena", "lighted", "bank", "quest",
    "prototype", "dnd"
};

const char             *const o_flags[] = {
    "glow", "hum", "dark", "loyal", "evil", "invis", "magic", "nodrop", "bless",
    "antigood", "antievil", "antineutral", "noremove", "inventory",
    "antimage", "antithief", "antiwarrior", "antipriest", "organic", "metal",
    "donation", "clanobject", "clancorpse", "antivampire", "antidruid",
    "hidden", "poisoned", "covering", "r22", "buried", "prototype",
    "nolocate", "groundrot", "lootable", "pkdisarmed", "lodged",
    "skelerot", "override", "clancontainer", "nogroup"
};

const char             *const w_flags[] = {
    "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
    "shield", "about", "waist", "wrist", "wield", "hold", "_dual_", "ears", "eyes",
    "missile", "back", "face", "ankle", "lodge_rib", "lodge_arm", "lodge_leg",
    "sheath", "shoulders"
};

const char             *const item_w_flags[] = {
    "take", "finger", "finger", "neck", "neck", "body", "head", "legs", "feet",
    "hands", "arms", "shield", "about", "waist", "wrist", "wrist", "wield",
    "hold", "dual", "ears", "eyes", "missile", "back", "face", "ankle", "ankle",
    "lodge_rib", "lodge_arm", "lodge_leg", "sheath", "shoulders"
};

const char             *const area_flags[] = {
    "antipkill", "freekill", "noteleport", "spelllimit", "darkness", "nopkill",
    "noastral", "nosummon", "noportal", "indoors", "unotsee", "noquest",
    "quest", "prototype", "nodiscovery", "lighted", "group", "noinfluence",
    "arena", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29",
    "r30", "r31"
};

const char             *const o_types[] = {
    "none", "light", "scroll", "wand", "staff", "weapon",
    "_fireweapon", "_missile", "treasure", "armor", "potion", "_worn",
    "furniture", "trash", "_oldtrap", "container", "_note", "drinkcon",
    "key", "food", "money", "pen", "boat", "corpse",
    "corpse_pc", "fountain", "pill", "blood", "bloodstain", "scraps",
    "pipe", "herbcon", "herb", "incense", "fire", "book",
    "switch", "lever", "pullchain", "button", "dial", "rune",
    "runepouch", "match", "trap", "map", "portal", "paper",
    "tinder", "lockpick", "spike", "disease", "oil", "fuel",
    "piece", "thowing", "missileweapon", "projectile", "quiver", "shovel",
    "salve", "cook", "keyring", "odor", "chance", "sharpen",
    "shackle", "raw", "instrument", "skeleton", "resource", "dye", "stone",
    "tool", "stove", "coal", "sheath", "sabotage", "forge"
};

/* Volk - a_types are basically affect types. These will include *
 * a number (ie affect strength 5). It also came with a pile of  *
 * skills/spells (ie backstab, disarm, brew). I removed these,   *
 * and instead specified use 'percentage' (sn) (number).         *
const char *const a_types[] = {
"none", "strength", "dexterity", "intelligence", "wisdom", "constitution",
"sex", "class", "level", "age", "height", "weight", "mana", "hit", "move",
"gold", "experience", "armor", "hitroll", "damroll", "save_poison", "save_rod",
"save_para", "save_breath", "save_spell", "charisma", "affected", "resistant",
"immune", "susceptible", "weaponspell", "luck", "backstab", "pick", "track",
"steal", "sneak", "hide", "palm", "detrap", "dodge", "peek", "scan", "gouge",
"search", "mount", "disarm", "kick", "parry", "bash", "stun", "punch", "climb",
"grip", "scribe", "brew", "wearspell", "removespell", "emotion", "mentalstate",
"stripsn", "remove", "dig", "full", "thirst", "drunk", "blood", "cook",
"recurringspell", "contagious", "xaffected", "odor", "roomflag", "sectortype",
"roomlight", "televnum", "teledelay", "phase", "displacement"
};
*/

const char             *const a_types[] = {
    "none", "strength", "dexterity", "intelligence", "wisdom", "constitution",  // (0- 5)
    "sex", "class", "race", "age", "height", "weight", "mana", "hit", "move",   // 14
    "unused", "unused", "armor", "hitroll", "damroll", "save_poison", "save_rod",
    "save_para", "save_breath", "save_spell", "charisma", "affected", "resistant",  // 27
    "immune", "susceptible", "weaponspell", "luck", "unused", "unused", "unused",   // 34
    "unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
    "unused",
    "unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",   // 52
    "unused", "unused", "unused", "wearspell", "removespell", "emotion", "mentalstate",
    "stripsn", "remove", "unused", "full", "thirst", "drunk", "blood", "unused",    // 67
    "recurringspell", "contagious", "xaffected", "odor", "roomflag", "sectortype",
    "roomlight", "televnum", "teledelay", "unused", "unused", "unused", "percentage"    // 80
};

const char             *const a_flags[] = {
    "blindness", "invisible", "detect_evil",
    "detect_invis", "detect_magic", "detect_hidden",
    "fascinate", "sanctuary", "faerie_fire",
    "infrared", "curse", "shield",                     // 12
    "poison", "protect", "paralysis",                  // 15
    "sneak", "hide", "sleep",                          // 18
    "charm", "flying", "pass_door",                    // 21
    "floating", "truesight", "detect_traps",
    "scrying", "fireshield", "shockshield",
    "detect_sneak", "iceshield", "possess",            // 30
    "berserk", "aqua_breath", "recurringspell",
    "contagious", "acidmist", "venomshield",
    "shapeshift", "demonic_sight", "nosight",
    "spike", "unholy_sphere", "wizard_eye",
    "thaitin", "shrink", "heavens_bless",              // 45
    "vampiric_strength", "surreal_speed", "burrow",
    "age", "silence", "feign", "mana_pool", "maim",
    "reactive", "nettle", "slow", "ward", "anoint",
    "fury", "boost", "prayer", "kinetic",
    "unsearing_skin", "iron_skin",                     // 64
    "defensive_posturing", "recoil", "sidestep",
    "thicken_skin", "root", "keen_eye", "murazor",
    "body", "stirring", "sustain_self", "snare",
    "ritual", "siphon_strength", "decree_decay",
    "giant_strength", "wizard_sight", "grendals_stance", "aura_life",
    "acidic_touch", "tangled", "flaming_shield", "divine_intervention", // 81
    "energy_containment",
// afflictions
    "fungal_toxin",
    "asthma", "clumsiness", "confusion", "dizziness", "nausea", "migraine",
    "frost_bite", "rotten_gut", "brittle_bones", "swelling", "blistering",
    "neurotoxin", "toxin", "corrosive", "venom_toxin", "highermagic",   // 98
    "ptalons", "battlefield", "diseased", "dragonlord", "hate", "crafted_food1", "crafted_food2",   // 105
    "crafted_food3", "crafted_drink1", "crafted_drink2", "crafted_drink3", "noscent", "victim", "graze",    // 112
    "graft", "ottos_dance", "sound_waves", "mounted", "phase", "tangle", "r119",    // 119
    "r120", "r121", "r122", "r123", "r124", "r125", "r126", "r127"  // 127
};

const char             *const act_flags[] = {
    "npc", "sentinel", "scavenger", "water", "noquest",
    "aggressive", "stayarea", "wimpy", "pet", "banker",
    "practice", "immortal", "deadly", "polyself", "wild_aggr",
    "guardian", "running", "nowander", "mountable", "mounted",
    "scholar", "secretive", "hardhat", "mobinvis", "noassist",
    "autonomous", "pacifist", "noattack", "annoying", "statshield",
    "prototype", "healer", "carry", "gralloc", "group",
    "questmaster", "undertaker", "override", "aclan_leader",
    "hclan_leader", "player", "newbpractice", "resizer", "nclan_leader",
    "questgiver", "sigilist", "craftshop", "max"
};

const char             *const pc_flags[] = {
/* changed "r8" to "" so players on watch can't see it  -- Gorog */
    "r1", "deadly", "unauthed", "norecall", "nointro",
    "gag", "retired", "guest", "nosummon", "pager",
    "notitled", "groupwho", "diagnose", "highgag", "",
    "hints", "dnd", "idle", "showslots", "admin",
    "enforcer", "builder", "temp", "privacy", "wizhcolor",
    "aidle", "r20", "nobeep", "demigod", "buildwalk"
};

const char             *const plr_flags[] = {
    "npc", "boughtpet", "shovedrag", "autoexits", "autoloot", "autotrash", "blank",
    "outcast", "brief", "combine", "prompt", "telnet_ga", "holylight",
    "wizinvis", "roomvnum", "silence", "noemote", "attacker", "notell", "log",  // 20
    "deny", "freeze", "thief", "killer", "litterbug", "ansi", "rip", "nice",
    "flee", "automoney", "staff", "afk", "invisprompt", "autoglance",
    "questing", "mxp", "shadowform", "exempt", "automap", "r40", "tutorial",
    "activate", "cartographer", "blind", "autodoor",   // 45
    "pksafe", "red", "blue", "frozen", "playing", "waiting", "life", "rp", "r54", "tease",
    "boat",
    "music", "sound",
    "noprune", "extreme", "r61", "r62", "r63", "r64", "r65", "r66", "r67", "r68", "r69",
    "r70", "r71", "r72", "r73", "r74", "r75", "r76", "r77", "r78", "r79", "r80", "r81"
};

const char             *const trap_flags[] = {
    "room", "obj", "enter", "leave", "open", "close", "get", "put", "pick",
    "unlock", "north", "south", "east", "r1", "west", "up", "down", "examine",
    "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13",
    "r14", "r15"
};

const char             *const cmd_flags[] = {
    "possessed", "polymorphed", "watch", "admin", "enforcer",
    "builder", "roleplayer", "fullname", "r7", "r8",
    "r9", "r10", "r11", "r12", "r13",
    "r14", "r15", "r16", "r17", "r18",
    "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28",
    "r29", "r30"
};

const char             *const wear_locs[] = {
    "light", "finger1", "finger2", "neck1", "neck2", "body", "head", "legs",
    "feet", "hands", "arms", "shield", "about", "waist", "wrist1", "wrist2",
    "wield", "hold", "dual_wield", "ears", "eyes", "missile_wield", "back",
    "face", "ankle1", "ankle2", "sheath", "shoulders"
};

const char             *const ris_flags[] = {
    "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
    "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
    "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "hack", "lash", "hit",
    "r4", "r5", "r6", "r7", "r8", "r9", "r10"
};

const char             *const trig_flags[] = {
    "up", "unlock", "lock", "d_north", "d_south", "d_east", "d_west", "d_up",
    "d_down", "door", "container", "open", "close", "passage", "oload", "mload",
    "teleport", "teleportall", "teleportplus", "death", "cast", "fakeblade",
    "rand4", "rand6", "trapdoor", "anotherroom", "usedial", "absolutevnum",
    "showroomdesc", "autoreturn", "r2", "r3"
};

const char             *const part_flags[] = {
    "head", "arms", "legs", "heart", "brains", "guts", "hands", "feet", "fingers",
    "ear", "eye", "long_tongue", "eyestalks", "tentacles", "fins", "wings",
    "tail", "scales", "claws", "fangs", "horns", "tusks", "tailattack",
    "sharpscales", "beak", "haunches", "hooves", "paws", "forelegs", "feathers",
    "r1", "r2"
};

const char             *const attack_flags[] = {
    "bite", "claws", "tail_swipe", "sting", "punch", "kick", "trip", "bash", "stun",
    "gouge", "backstab", "feed", "drain", "firebreath", "frostbreath",
    "acidbreath", "lightnbreath", "gasbreath", "poison", "nastypoison", "gaze",
    "blindness", "causeserious", "earthquake", "causecritical", "curse",
    "flamestrike", "harm", "fireball", "colorspray", "weaken", "spiralblast",
    "inferno", "blizzard", "ebolt", "blightning", "brainboil", "gsmite", "hhands",
    "ltouch", "max"
};

const char             *const defense_flags[] = {
    "parry", "dodge", "heal", "minorhealing", "archhealing", "sylvanwind",
    "dispelmagic", "dispelevil", "sanctuary", "fireshield", "shockshield",
    "shield", "bless", "stoneskin", "teleport", "acompanion", "lskellie", "celemental",
    "sward", "disarm", "iceshield", "grip", "truesight", "acidmist", "venomshield",
    "bestowvitae", "phase", "shieldblock", "displacement", "gheal",
    "counterstrike", "blademaster", "dbless", "r13", "r14"
};

/*
 * Note: I put them all in one big set of flags since almost all of these
 * can be shared between mobs, objs and rooms for the exception of
 * bribe and hitprcnt, which will probably only be used on mobs.
 * ie: drop -- for an object, it would be triggered when that object is
 * dropped; -- for a room, it would be triggered when anything is dropped
 *          -- for a mob, it would be triggered when anything is dropped
 *
 * Something to consider: some of these triggers can be grouped together,
 * and differentiated by different arguments... for example:
 *  hour and time, rand and randiw, speech and speechiw
 * 
 */
const char             *const mprog_flags[] = {
    "act", "speech", "rand", "fight", "death", "hitprcnt", "entry", "greet",
    "allgreet", "give", "bribe", "hour", "time", "wear", "remove", "trash",
    "look", "exa", "zap", "get", "drop", "damage", "repair", "randiw",
    "speechiw", "pull", "push", "sleep", "rest", "leave", "script", "use",
    "preenter"
};

char                   *flag_string( int bitvector, const char *const flagarray[] )
{
    static char             buf[MSL];
    int                     x;

    buf[0] = '\0';
    for ( x = 0; x < 32; x++ )
        if ( IS_SET( bitvector, 1 << x ) ) {
            mudstrlcat( buf, flagarray[x], MSL );
            /*
             * don't catenate a blank if the last char is blank  --Gorog 
             */
            if ( buf[0] != '\0' && ' ' != buf[strlen( buf ) - 1] )
                mudstrlcat( buf, " ", MSL );
        }
    if ( ( x = strlen( buf ) ) > 0 )
        buf[--x] = '\0';

    return buf;
}

char                   *ext_flag_string( EXT_BV * bitvector, const char *const flagarray[] )
{
    static char             buf[MSL];
    int                     x;

    buf[0] = '\0';
    for ( x = 0; x < MAX_BITS; x++ )
        if ( xIS_SET( *bitvector, x ) ) {
            mudstrlcat( buf, flagarray[x], MSL );
            mudstrlcat( buf, " ", MSL );
        }
    if ( ( x = strlen( buf ) ) > 0 )
        buf[--x] = '\0';

    return buf;
}

bool can_rmodify( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
{
    int                     vnum = room->vnum;
    AREA_DATA              *pArea;

    if ( IS_NPC( ch ) )
        return FALSE;
    if ( get_trust( ch ) >= sysdata.level_modify_proto )
        return TRUE;
    if ( !IS_SET( room->room_flags, ROOM_PROTOTYPE ) ) {
        send_to_char( "You cannot modify this room.\r\n", ch );
        return FALSE;
    }
    if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
        send_to_char( "You must have an assigned area to modify this room.\r\n", ch );
        return FALSE;
    }
    if ( vnum >= pArea->low_r_vnum && vnum <= pArea->hi_r_vnum )
        return TRUE;
    send_to_char( "That room is not in your allocated range.\r\n", ch );
    return FALSE;
}

bool can_omodify( CHAR_DATA *ch, OBJ_DATA *obj )
{
    int                     vnum = obj->pIndexData->vnum;
    AREA_DATA              *pArea;

    if ( IS_NPC( ch ) )
        return FALSE;
    if ( get_trust( ch ) >= sysdata.level_modify_proto )
        return TRUE;
    if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        send_to_char( "You cannot modify this object.\r\n", ch );
        return FALSE;
    }
    if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
        send_to_char( "You must have an assigned area to modify this object.\r\n", ch );
        return FALSE;
    }
    if ( vnum >= pArea->low_o_vnum && vnum <= pArea->hi_o_vnum )
        return TRUE;
    send_to_char( "That object is not in your allocated range.\r\n", ch );
    return FALSE;
}

bool can_oedit( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
    int                     vnum = obj->vnum;
    AREA_DATA              *pArea;

    if ( IS_NPC( ch ) )
        return FALSE;
    if ( get_trust( ch ) >= LEVEL_IMMORTAL )
        return TRUE;
    if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        send_to_char( "You cannot modify this object.\r\n", ch );
        return FALSE;
    }
    if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
        send_to_char( "You must have an assigned area to modify this object.\r\n", ch );
        return FALSE;
    }
    if ( vnum >= pArea->low_o_vnum && vnum <= pArea->hi_o_vnum )
        return TRUE;
    send_to_char( "That object is not in your allocated range.\r\n", ch );
    return FALSE;
}

bool can_mmodify( CHAR_DATA *ch, CHAR_DATA *mob )
{
    int                     vnum;
    AREA_DATA              *pArea;

    if ( mob == ch )
        return TRUE;

    if ( !IS_NPC( mob ) ) {
        if ( get_trust( ch ) >= sysdata.level_modify_proto && get_trust( ch ) > get_trust( mob ) )
            return TRUE;
        else
            send_to_char( "You can't do that.\r\n", ch );
        return FALSE;
    }

    vnum = mob->pIndexData->vnum;

    if ( IS_NPC( ch ) )
        return FALSE;
    if ( get_trust( ch ) >= sysdata.level_modify_proto )
        return TRUE;
    if ( !xIS_SET( mob->act, ACT_PROTOTYPE ) ) {
        send_to_char( "You cannot modify this mobile.\r\n", ch );
        return FALSE;
    }
    if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
        send_to_char( "You must have an assigned area to modify this mobile.\r\n", ch );
        return FALSE;
    }
    if ( vnum >= pArea->low_m_vnum && vnum <= pArea->hi_m_vnum )
        return TRUE;

    send_to_char( "That mobile is not in your allocated range.\r\n", ch );
    return FALSE;
}

bool can_medit( CHAR_DATA *ch, MOB_INDEX_DATA *mob )
{
    int                     vnum = mob->vnum;
    AREA_DATA              *pArea;

    if ( IS_NPC( ch ) )
        return FALSE;
    if ( get_trust( ch ) >= LEVEL_IMMORTAL )
        return TRUE;
    if ( !xIS_SET( mob->act, ACT_PROTOTYPE ) ) {
        send_to_char( "You cannot modify this mobile.\r\n", ch );
        return FALSE;
    }
    if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
        send_to_char( "You must have an assigned area to modify this mobile.\r\n", ch );
        return FALSE;
    }
    if ( vnum >= pArea->low_m_vnum && vnum <= pArea->hi_m_vnum )
        return TRUE;

    send_to_char( "That mobile is not in your allocated range.\r\n", ch );
    return FALSE;
}

//Next two functions were made to handle assigning min/max damage
//according to level passed through oset. -Taon
short set_max_chart( int level )
{
    short                   num_dice;

    if ( level <= 4 )
        num_dice = 5;
    else if ( level <= 9 )
        num_dice = 10;
    else if ( level <= 19 )
        num_dice = 15;
    else if ( level <= 24 )
        num_dice = 20;
    else if ( level <= 34 )
        num_dice = 30;
    else if ( level <= 39 )
        num_dice = 35;
    else if ( level <= 44 )
        num_dice = 40;
    else if ( level <= 49 )
        num_dice = 45;
    else if ( level <= 54 )
        num_dice = 50;
    else if ( level <= 60 )
        num_dice = 60;
    else if ( level <= 70 )
        num_dice = 70;
    else if ( level <= 80 )
        num_dice = 80;
    else if ( level <= 90 )
        num_dice = 90;
    else if ( level <= 99 )
        num_dice = 100;
    else
        num_dice = 100;

    return num_dice;
}

short set_min_chart( int level )
{
    short                   size_dice;

    if ( level <= 4 )
        size_dice = 1;
    else if ( level <= 9 )
        size_dice = 3;
    else if ( level <= 19 )
        size_dice = 5;
    else if ( level <= 25 )
        size_dice = 10;
    else if ( level <= 34 )
        size_dice = 15;
    else if ( level <= 44 )
        size_dice = 20;
    else if ( level <= 54 )
        size_dice = 25;
    else if ( level <= 64 )
        size_dice = 30;
    else if ( level <= 74 )
        size_dice = 35;
    else if ( level <= 84 )
        size_dice = 40;
    else if ( level <= 99 )
        size_dice = 45;
    else
        size_dice = 50;

    return size_dice;
}

short min_archery_chart( int level )
{
    short                   size_dice;

    if ( level <= 4 )
        size_dice = 1;
    else if ( level <= 9 )
        size_dice = 2;
    else if ( level <= 19 )
        size_dice = 5;
    else if ( level <= 25 )
        size_dice = 3;
    else if ( level <= 34 )
        size_dice = 4;
    else if ( level <= 44 )
        size_dice = 5;
    else if ( level <= 54 )
        size_dice = 6;
    else if ( level <= 64 )
        size_dice = 7;
    else if ( level <= 74 )
        size_dice = 8;
    else if ( level <= 84 )
        size_dice = 9;
    else if ( level <= 99 )
        size_dice = 10;
    else
        size_dice = 10;

    return size_dice;
}

short max_archery_chart( int level )
{
    short                   size_dice;

    if ( level <= 4 )
        size_dice = 4;
    else if ( level <= 9 )
        size_dice = 8;
    else if ( level <= 19 )
        size_dice = 10;
    else if ( level <= 25 )
        size_dice = 13;
    else if ( level <= 34 )
        size_dice = 14;
    else if ( level <= 44 )
        size_dice = 15;
    else if ( level <= 54 )
        size_dice = 16;
    else if ( level <= 64 )
        size_dice = 17;
    else if ( level <= 74 )
        size_dice = 18;
    else if ( level <= 84 )
        size_dice = 19;
    else if ( level <= 99 )
        size_dice = 20;
    else
        size_dice = 20;

    return size_dice;
}

short set_min_armor( int level )
{
    short                   value;

    if ( level <= 4 )
        value = 2;
    else if ( level <= 9 )
        value = 5;
    else if ( level <= 19 )
        value = 10;
    else if ( level <= 25 )
        value = 20;
    else if ( level <= 34 )
        value = 30;
    else if ( level <= 44 )
        value = 35;
    else if ( level <= 54 )
        value = 40;
    else if ( level <= 64 )
        value = 45;
    else if ( level <= 74 )
        value = 50;
    else if ( level <= 84 )
        value = 55;
    else
        value = 60;

    return value;
}

short set_max_armor( int level )
{
    short                   value;

    if ( level <= 4 )
        value = 2;
    else if ( level <= 9 )
        value = 5;
    else if ( level <= 19 )
        value = 10;
    else if ( level <= 25 )
        value = 20;
    else if ( level <= 34 )
        value = 30;
    else if ( level <= 44 )
        value = 35;
    else if ( level <= 54 )
        value = 40;
    else if ( level <= 64 )
        value = 45;
    else if ( level <= 74 )
        value = 50;
    else if ( level <= 84 )
        value = 55;
    else
        value = 60;

    return value;
}

//Set number of attacks, getting ready for changes to mset. -Taon
short set_num_attacks( int level )
{
    short                   attacks;

    if ( level < 5 )
        attacks = 0;
    else if ( level <= 10 )
        attacks = 1;
    else if ( level <= 15 )
        attacks = 2;
    else if ( level <= 25 )
        attacks = 3;
    else if ( level <= 35 )
        attacks = 4;
    else if ( level <= 45 )
        attacks = 5;
    else if ( level <= 55 )
        attacks = 6;
    else if ( level <= 65 )
        attacks = 7;
    else
        attacks = 8;

    return attacks;
}

// Autoset damroll based upon level.
int set_damroll( int level )
{
    int                     damroll;

    if ( level < 6 )
        damroll = 1;
    else if ( level <= 8 )
        damroll = 3;
    else if ( level <= 10 )
        damroll = 5;
    else if ( level <= 15 )
        damroll = 10;
    else if ( level <= 20 )
        damroll = 15;
    else if ( level <= 30 )
        damroll = 18;
    else if ( level <= 40 )
        damroll = 20;
    else if ( level <= 50 )
        damroll = 30;
    else if ( level <= 60 )
        damroll = 40;
    else if ( level <= 70 )
        damroll = 90;
    else if ( level <= 80 )
        damroll = 150;
    else if ( level <= 90 )
        damroll = 190;
    else if ( level <= 95 )
        damroll = 200;
    else
        damroll = 200;

    return damroll;
}

// Autoset hitroll based upon level.
int set_hitroll( int level )
{
    int                     hitroll;

    if ( level < 6 )
        hitroll = 1;
    else if ( level <= 8 )
        hitroll = 3;
    else if ( level <= 10 )
        hitroll = 5;
    else if ( level <= 15 )
        hitroll = 10;
    else if ( level <= 20 )
        hitroll = 15;
    else if ( level <= 30 )
        hitroll = 20;
    else if ( level <= 40 )
        hitroll = 30;
    else if ( level <= 50 )
        hitroll = 35;
    else if ( level <= 60 )
        hitroll = 40;
    else if ( level <= 70 )
        hitroll = 50;
    else if ( level <= 80 )
        hitroll = 140;
    else if ( level <= 90 )
        hitroll = 170;
    else if ( level <= 95 )
        hitroll = 200;
    else
        hitroll = 200;

    return hitroll;
}

//Declare the amt of money a mob is carring, varying on level
//and currtype. This is so mean..... Vladaar made me do it! -Taon
//Status/Information: This is probably going to be junked, just thought 
//of a much better solution. -Taon
int set_mob_currency( int level, int ctype )
{
    int                     amt;

    if ( level < 10 )
        amt = number_chance( 1, 15 );
    else if ( level < 20 )
        amt = number_chance( 5, 20 );
    else if ( level < 30 )
        amt = number_chance( 10, 25 );
    else if ( level < 40 )
        amt = number_chance( 10, 50 );
    else if ( level < 50 )
        amt = number_range( 20, 100 );
    else if ( level < 60 )
        amt = number_range( 20, 150 );
    else if ( level < 70 )
        amt = number_range( 30, 200 );
    else if ( level < 80 )
        amt = number_range( 30, 250 );
    else if ( level < 90 )
        amt = number_range( 30, 300 );
    else
        amt = number_range( 40, 400 );

    if ( ctype == CURR_SILVER ) {
        if ( level < 50 )
            amt /= 4;
        else
            amt /= 2;
    }
    else if ( ctype == CURR_GOLD ) {
        if ( level < 10 )
            amt = 1;
        else if ( level < 25 )
            amt /= 5;
        else if ( level < 50 )
            amt /= 4;
        else
            amt /= 3;
    }
    return amt;
}

// Autoset hps based upon level. -Taon
int set_hp( int level )
{
    int                     hp;

    if ( level < 4 )
        hp = 20;
    else if ( level <= 5 )
        hp = 22;
    else if ( level <= 6 )
        hp = 24;
    else if ( level <= 6 )
        hp = 26;
    else if ( level <= 8 )
        hp = 30;
    else if ( level <= 9 )
        hp = 50;
    else if ( level <= 10 )
        hp = 80;
    else if ( level <= 11 )
        hp = 90;
    else if ( level <= 12 )
        hp = 100;
    else if ( level <= 13 )
        hp = 150;
    else if ( level <= 14 )
        hp = 200;
    else if ( level <= 15 )
        hp = 300;
    else if ( level <= 16 )
        hp = 350;
    else if ( level <= 17 )
        hp = 400;
    else if ( level <= 18 )
        hp = 500;
    else if ( level <= 19 )
        hp = 700;
    else if ( level <= 20 )
        hp = 750;
    else if ( level <= 25 )
        hp = 800;
    else if ( level <= 30 )
        hp = 850;
    else if ( level <= 35 )
        hp = 900;
    else if ( level <= 40 )
        hp = 1000;
    else if ( level <= 45 )
        hp = 1600;
    else if ( level <= 50 )
        hp = 2000;
    else if ( level <= 55 )
        hp = 2300;
    else if ( level <= 60 )
        hp = 3000;
    else if ( level <= 65 )
        hp = 4300;
    else if ( level <= 75 )
        hp = 5500;
    else if ( level <= 80 )
        hp = 7200;
    else if ( level <= 85 )
        hp = 8500;
    else if ( level <= 90 )
        hp = 9500;
    else if ( level <= 95 )
        hp = 10000;
    else
        hp = 10000;

    return hp;
}

//Return value of armor class for mset in level argument. -Taon
int set_armor_class( int level )
{
    int                     ac;

    if ( level < 5 )
        ac = 300;
    else if ( level <= 6 )
        ac = 290;
    else if ( level <= 7 )
        ac = 280;
    else if ( level <= 8 )
        ac = 270;
    else if ( level <= 9 )
        ac = 260;
    else if ( level <= 10 )
        ac = 250;
    else if ( level <= 12 )
        ac = 240;
    else if ( level <= 13 )
        ac = 235;
    else if ( level <= 15 )
        ac = 230;
    else if ( level <= 20 )
        ac = 180;
    else if ( level <= 25 )
        ac = 150;
    else if ( level <= 30 )
        ac = 100;
    else if ( level <= 35 )
        ac = 0;
    else if ( level <= 40 )
        ac = -50;
    else if ( level <= 45 )
        ac = -80;
    else if ( level <= 50 )
        ac = -150;
    else if ( level <= 55 )
        ac = -200;
    else if ( level <= 60 )
        ac = -300;
    else if ( level <= 65 )
        ac = -400;
    else if ( level <= 70 )
        ac = -550;
    else if ( level <= 75 )
        ac = -680;
    else if ( level <= 80 )
        ac = -700;
    else if ( level <= 85 )
        ac = -820;
    else if ( level <= 90 )
        ac = -950;
    else if ( level <= 95 )
        ac = -980;
    else
        ac = -1000;

    return ac;
}

//Establish currency according to item type and level. -Taon
short set_curr_type( OBJ_DATA *obj, int level )
{
    short                   curr;

    switch ( obj->item_type ) {                        // Allow fall throughs in this
            // switch. -Taon
        case ITEM_ARMOR:

            if ( obj->level < 20 )
                curr = CURR_COPPER;
            else if ( obj->level < 40 )
                curr = CURR_BRONZE;
            else if ( obj->level < 60 )
                curr = CURR_SILVER;
            else
                curr = CURR_GOLD;

            break;
        case ITEM_CONTAINER:

            if ( obj->level < 40 )
                curr = CURR_COPPER;
            else if ( obj->level < 70 )
                curr = CURR_BRONZE;
            else
                curr = CURR_SILVER;

            break;
        case ITEM_PILL:
        case ITEM_POTION:

            if ( obj->level < 25 )
                curr = CURR_COPPER;
            else if ( obj->level < 60 )
                curr = CURR_BRONZE;
            else if ( obj->level < 85 )
                curr = CURR_SILVER;
            else
                curr = CURR_GOLD;

            break;
        case ITEM_SCROLL:
        case ITEM_SALVE:
            if ( obj->level < 40 )
                curr = CURR_BRONZE;
            else if ( obj->level < 80 )
                curr = CURR_SILVER;
            else
                curr = CURR_GOLD;

            break;
        case ITEM_TREASURE:
            curr = CURR_GOLD;
            break;
        case ITEM_WEAPON:
        case ITEM_MISSILE:

            if ( obj->level < 20 )
                curr = CURR_COPPER;
            else if ( obj->level < 40 )
                curr = CURR_BRONZE;
            else if ( obj->level < 60 )
                curr = CURR_SILVER;
            else
                curr = CURR_GOLD;

            break;
        case ITEM_DYE:
        case ITEM_TRASH:
            curr = CURR_COPPER;
            break;
        case ITEM_PIPE:

            if ( obj->level < 50 )
                curr = CURR_COPPER;
            else
                curr = CURR_BRONZE;

            break;
        case ITEM_STAFF:
        case ITEM_WAND:

            if ( obj->level < 50 )
                curr = CURR_SILVER;
            else
                curr = CURR_GOLD;

            break;
        default:
            // If we left any out we'll allow them to set to default value. -Taon
            curr = CURR_COPPER;
            break;
    }

    return curr;
}

//Establish amt by currtype and object level. -Taon
int set_curr_amt( OBJ_DATA *obj, int level )
{
    int                     curr_amt;

    if ( obj->item_type == ITEM_TRASH )
        return 0;

    if ( obj->currtype == CURR_BRONZE || obj->currtype == CURR_COPPER ) {
        if ( level < 10 )
            curr_amt = number_chance( 10, 30 );
        else if ( level < 20 )
            curr_amt = number_chance( 30, 90 );
        else if ( level < 30 )
            curr_amt = number_range( 90, 150 );
        else if ( level < 40 )
            curr_amt = number_range( 150, 225 );
        else if ( level < 50 )
            curr_amt = number_range( 225, 300 );
        else if ( level < 60 )
            curr_amt = number_range( 300, 400 );
        else if ( level < 70 )
            curr_amt = number_range( 400, 600 );
        else if ( level < 80 )
            curr_amt = number_range( 600, 800 );
        else if ( level < 90 )
            curr_amt = number_range( 800, 1000 );
        else if ( level <= 95 )
            curr_amt = number_range( 1000, 1250 );
        else
            curr_amt = number_range( 1250, 1500 );
    }
    else if ( obj->currtype == CURR_GOLD || obj->currtype == DEFAULT_CURR ) {
        if ( level < 20 )
            curr_amt = number_chance( 1, 5 );
        else if ( level < 30 )
            curr_amt = number_chance( 5, 10 );
        else if ( level < 40 )
            curr_amt = number_chance( 10, 20 );
        else if ( level < 50 )
            curr_amt = number_chance( 20, 30 );
        else if ( level < 60 )
            curr_amt = number_chance( 30, 40 );
        else if ( level < 70 )
            curr_amt = number_chance( 40, 50 );
        else if ( level < 80 )
            curr_amt = number_chance( 50, 60 );
        else if ( level < 90 )
            curr_amt = number_chance( 60, 75 );
        else if ( level <= 95 )
            curr_amt = number_chance( 75, 95 );
        else
            curr_amt = number_range( 95, 150 );
    }
    else if ( obj->currtype == CURR_SILVER ) {
        if ( level < 20 )
            curr_amt = number_chance( 1, 10 );
        else if ( level < 30 )
            curr_amt = number_chance( 10, 20 );
        else if ( level < 40 )
            curr_amt = number_chance( 20, 30 );
        else if ( level < 50 )
            curr_amt = number_chance( 30, 40 );
        else if ( level < 60 )
            curr_amt = number_chance( 40, 55 );
        else if ( level < 70 )
            curr_amt = number_chance( 55, 70 );
        else if ( level < 80 )
            curr_amt = number_chance( 70, 90 );
        else if ( level < 90 )
            curr_amt = number_range( 90, 120 );
        else if ( level <= 95 )
            curr_amt = number_range( 120, 150 );
        else
            curr_amt = number_range( 150, 200 );
    }
    else {
        log_printf( "Error bad currtype detected on [name: %s]", obj->short_descr );
        return 0;
    }

    // Adjust some Items through currtype. -Taon
    switch ( obj->item_type ) {                        // Allow fall throughs in this
            // switch.
        case ITEM_TREASURE:
            if ( obj->level < 30 )
                curr_amt *= 2;
            else if ( obj->level < 60 )
                curr_amt *= 3;
            else if ( obj->level < 90 )
                curr_amt *= 4;
            else
                curr_amt *= 5;
            break;
        case ITEM_ARMOR:
            if ( obj->level < 20 )
                curr_amt += number_chance( 25, 40 );
            else if ( obj->level < 40 )
                curr_amt += number_chance( 40, 60 );
            else if ( obj->level < 60 )
                curr_amt += number_chance( 60, 80 );
            else if ( obj->level < 80 )
                curr_amt += number_range( 80, 110 );
            else
                curr_amt += number_range( 100, 200 );
            break;
        case ITEM_MISSILE:
        case ITEM_WEAPON:
            if ( obj->level < 20 )
                curr_amt += number_chance( 10, 20 );
            else if ( obj->level < 40 )
                curr_amt += number_chance( 20, 40 );
            else if ( obj->level < 60 )
                curr_amt += number_chance( 40, 60 );
            else if ( obj->level < 80 )
                curr_amt += number_chance( 60, 90 );
            else
                curr_amt += number_range( 90, 120 );
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            if ( obj->level < 30 )
                curr_amt += number_chance( 5, 15 );
            else if ( obj->level < 60 )
                curr_amt += number_chance( 15, 25 );
            else if ( obj->level < 90 )
                curr_amt += number_chance( 25, 40 );
            else
                curr_amt += number_chance( 40, 65 );
            break;
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_SALVE:
            curr_amt /= 2;
            break;
        default:
            break;
    }

    // Too much gold has been reached, log it. -Taon
    if ( curr_amt > 1000 && obj->currtype == CURR_GOLD && !IS_OBJ_STAT( obj, ITEM_OVERRIDE ) )
        log_printf( "Object:[%s] spawned with %d gold ", obj->name, curr_amt );

    return curr_amt;
}

int get_otype( char *type )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( o_types ) / sizeof( o_types[0] ) ); x++ )
        if ( !str_cmp( type, o_types[x] ) )
            return x;
    return -1;
}

int get_aflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( a_flags ) / sizeof( a_flags[0] ) ); x++ )
        if ( !str_cmp( flag, a_flags[x] ) )
            return x;
    return -1;
}

int get_trapflag( char *flag )
{
    size_t                  x;

    for ( x = 0; x < ( sizeof( trap_flags ) / sizeof( trap_flags[0] ) ); x++ )
        if ( !str_cmp( flag, trap_flags[x] ) )
            return x;
    return -1;
}

int get_atype( char *type )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( a_types ) / sizeof( a_types[0] ) ); x++ )
        if ( !str_cmp( type, a_types[x] ) )
            return x;
    return -1;
}

extern int              get_npc_class( char *Class );
extern int              get_npc_race( char *type );
extern int              get_pc_race( char *type );

int get_npc_class( char *Class )
{
    unsigned int            x;

    for ( x = 0; x < MAX_NPC_CLASS; x++ )
        if ( !str_cmp( Class, npc_class[x] ) )
            return x;
    return -1;
}

int get_npc_race( char *type )
{
    unsigned int            x;

    for ( x = 0; x < MAX_NPC_RACE; x++ )
        if ( !str_cmp( type, npc_race[x] ) )
            return x;
    return -1;
}

int get_pc_race( char *type )
{
    int                     i;

    for ( i = 0; i < MAX_RACE; i++ )
        if ( !str_cmp( type, race_table[i]->race_name ) )
            return i;
    return -1;
}

int get_wearloc( char *type )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( wear_locs ) / sizeof( wear_locs[0] ) ); x++ )
        if ( !str_cmp( type, wear_locs[x] ) )
            return x;
    return -1;
}

int get_secflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( sec_flags ) / sizeof( sec_flags[0] ) ); x++ )
        if ( !str_cmp( flag, sec_flags[x] ) )
            return x;
    return -1;
}

int get_exflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( ex_flags ) / sizeof( ex_flags[0] ) ); x++ )
        if ( !str_cmp( flag, ex_flags[x] ) )
            return x;
    return -1;
}

int get_pulltype( char *type )
{
    unsigned int            x;

    if ( !str_cmp( type, "none" ) || !str_cmp( type, "clear" ) )
        return 0;

    for ( x = 0; x < ( sizeof( ex_pmisc ) / sizeof( ex_pmisc[0] ) ); x++ )
        if ( !str_cmp( type, ex_pmisc[x] ) )
            return x;

    for ( x = 0; x < ( sizeof( ex_pwater ) / sizeof( ex_pwater[0] ) ); x++ )
        if ( !str_cmp( type, ex_pwater[x] ) )
            return x + PT_WATER;
    for ( x = 0; x < ( sizeof( ex_pair ) / sizeof( ex_pair[0] ) ); x++ )
        if ( !str_cmp( type, ex_pair[x] ) )
            return x + PT_AIR;
    for ( x = 0; x < ( sizeof( ex_pearth ) / sizeof( ex_pearth[0] ) ); x++ )
        if ( !str_cmp( type, ex_pearth[x] ) )
            return x + PT_EARTH;
    for ( x = 0; x < ( sizeof( ex_pfire ) / sizeof( ex_pfire[0] ) ); x++ )
        if ( !str_cmp( type, ex_pfire[x] ) )
            return x + PT_FIRE;
    return -1;
}

int get_rflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( r_flags ) / sizeof( r_flags[0] ) ); x++ )
        if ( !str_cmp( flag, r_flags[x] ) )
            return x;
    return -1;
}

int get_mpflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( mprog_flags ) / sizeof( mprog_flags[0] ) ); x++ )
        if ( !str_cmp( flag, mprog_flags[x] ) )
            return x;
    return -1;
}

int get_oflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( o_flags ) / sizeof( o_flags[0] ) ); x++ )
        if ( !str_cmp( flag, o_flags[x] ) )
            return x;
    return -1;
}

int get_areaflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( area_flags ) / sizeof( area_flags[0] ) ); x++ )
        if ( !str_cmp( flag, area_flags[x] ) )
            return x;
    return -1;
}

int get_trigflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( trig_flags ) / sizeof( trig_flags[0] ) ); x++ )
        if ( !str_cmp( flag, trig_flags[x] ) )
            return x;
    return -1;
}

int get_partflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( part_flags ) / sizeof( part_flags[0] ) ); x++ )
        if ( !str_cmp( flag, part_flags[x] ) )
            return x;
    return -1;
}

int get_attackflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( attack_flags ) / sizeof( attack_flags[0] ) ); x++ )
        if ( !str_cmp( flag, attack_flags[x] ) )
            return x;
    return -1;
}

int get_defenseflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( defense_flags ) / sizeof( defense_flags[0] ) ); x++ )
        if ( !str_cmp( flag, defense_flags[x] ) )
            return x;
    return -1;
}

int get_langnum( char *flag )
{
    unsigned int            x;

    for ( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
        if ( !str_cmp( flag, lang_names[x] ) )
            return x;
    return -1;
}

int get_wflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( w_flags ) / sizeof( w_flags[0] ) ); x++ )
        if ( !str_cmp( flag, w_flags[x] ) )
            return x;
    return -1;
}

int get_actflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( act_flags ) / sizeof( act_flags[0] ) ); x++ )
        if ( !str_cmp( flag, act_flags[x] ) )
            return x;
    return -1;
}

int get_pcflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( pc_flags ) / sizeof( pc_flags[0] ) ); x++ )
        if ( !str_cmp( flag, pc_flags[x] ) )
            return x;
    return -1;
}

int get_plrflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( plr_flags ) / sizeof( plr_flags[0] ) ); x++ )
        if ( !str_cmp( flag, plr_flags[x] ) )
            return x;
    return -1;
}

int get_risflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( ris_flags ) / sizeof( ris_flags[0] ) ); x++ )
        if ( !str_cmp( flag, ris_flags[x] ) )
            return x;
    return -1;
}

/*
 * For use with cedit --Shaddai
 */

int get_cmdflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( cmd_flags ) / sizeof( cmd_flags[0] ) ); x++ )
        if ( !str_cmp( flag, cmd_flags[x] ) )
            return x;
    return -1;
}

int get_langflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
        if ( !str_cmp( flag, lang_names[x] ) )
            return lang_array[x];
    return LANG_UNKNOWN;
}

/* Remove carriage returns from a line */
char                   *strip_cr( char *str )
{
    static char             newstr[MSL];
    int                     i,
                            j;

    if ( !str )
        return str;
    for ( i = j = 0; str[i] != '\0'; i++ )
        if ( str[i] != '\r' ) {
            newstr[j++] = str[i];
        }
    newstr[j] = '\0';
    return newstr;
}

void do_goto( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location,
                           *in_room;
    CHAR_DATA              *fch,
                           *fch_next,
                           *victim;
    AREA_DATA              *pArea;
    int                     vnum;

    /*
     * Deal with switched imms 
     */
    if ( ch->desc && ch->desc->original ) {
        interpret( ch, ( char * ) "return" );
        return;
    }
    if ( ch->position == POS_CRAWL || ch->position == POS_CROUCH ) {
        send_to_char
            ( "That would affect your height in the negative you will need to stand first.\r\n",
              ch );
        return;
    }
    one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Goto where?\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "next" ) || !str_cmp( arg, "prev" ) ) {
        int                     nextRoom = 0;
        char                    temp[15];

        if ( !ch->in_room || !ch->in_room->vnum ) {
            bug( "do_goto: %s is not in a room... um... this is BAD", ch->name );
            return;
        }
        nextRoom = ch->in_room->vnum;
        if ( !str_cmp( arg, "next" ) )
            nextRoom++;
        if ( !str_cmp( arg, "prev" ) )
            nextRoom--;
        snprintf( temp, 15, "%d", nextRoom );
        do_goto( ch, temp );
        return;
    }
    if ( !is_number( arg ) && ( fch = get_char_world( ch, arg ) ) ) {
        if ( str_cmp( ch->name, "Vladaar" ) )
            if ( !IS_NPC( fch ) && get_trust( ch ) < get_trust( fch ) ) {
                pager_printf( ch, "If you want to be in %s's presence ask permission first.\r\n",
                              fch->name );
                pager_printf( fch,
                              "%s has tried to goto you, give them the room vnum if you want them there.\r\n",
                              ch->name );
                return;
            }
    }
    if ( ( location = find_location( ch, arg ) ) == NULL ) {
        vnum = atoi( arg );
        if ( vnum < 0 || get_room_index( vnum ) ) {
            send_to_char( "You cannot find that...\r\n", ch );
            return;
        }
        if ( vnum < 1 || vnum > MAX_VNUM || IS_NPC( ch ) || !ch->pcdata->area ) {
            send_to_char( "No such location.\r\n", ch );
            return;
        }
        if ( !ch->pcdata->area && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "No such location.\r\n", ch );
            return;
        }
        if ( get_trust( ch ) < sysdata.level_modify_proto ) {
            if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
                send_to_char( "You must have an assigned area to create rooms.\r\n", ch );
                return;
            }
            if ( vnum < pArea->low_r_vnum || vnum > pArea->hi_r_vnum ) {
                send_to_char( "That room is not within your assigned range.\r\n", ch );
                return;
            }
        }
        location = make_room( vnum, ch->pcdata->area );
        if ( !location ) {
            bug( "Goto: make_room failed" );
            return;
        }
        location->area = ch->pcdata->area;
        set_char_color( AT_WHITE, ch );
        send_to_char
            ( "Waving your hand, you form order from swirling chaos,\r\nand step into a new reality...\r\n",
              ch );
    }
    if ( ( victim = room_is_dnd( ch, location ) ) ) {
        send_to_pager( "That room is \"do not disturb\" right now.\r\n", ch );
        pager_printf( victim, "Your DND flag just foiled %s's goto command.\r\n", ch->name );
        return;
    }
    if ( room_is_private( location ) ) {
        if ( get_trust( ch ) < sysdata.level_override_private ) {
            send_to_char( "That room is private right now.\r\n", ch );
            return;
        }
        else
            send_to_char( "Overriding private flag!\r\n", ch );
    }
    in_room = ch->in_room;
    if ( ch->fighting )
        stop_fighting( ch, TRUE );
    if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) && !IS_NPC( ch ) ) {
        if ( ch->level == 108 && !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
            act( AT_CYAN,
                 "\r\n&YThe sounds of bones popping fills yours ears as $n suddenly morphs into a raven shape!\r\n\r\n&cA raven flies into the air!&D\r\n\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        else if ( !xIS_SET( ch->act, PLR_WIZINVIS ) && ch->level < 108 ) {
            if ( !VLD_STR( ch->pcdata->bamfout ) )
                act( AT_IMMORT,
                     "&C[OoC]: $n please &Rvote&C for us at &Yhttp://6dragons.org/bin/vote.html &Cthis message suddenly &REXPLODES!&c and $n is GONE!&D",
                     ch, NULL, NULL, TO_ROOM );
            else
                act( AT_IMMORT, ch->pcdata->bamfout, ch, NULL, NULL, TO_ROOM );
        }
    }
    ch->regoto = ch->in_room->vnum;
    char_from_room( ch );
    if ( ch->mount ) {
        char_from_room( ch->mount );
        char_to_room( ch->mount, location );
    }
// bah summoned pets with goto
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( found && xIS_SET( mob->act, ACT_PET ) ) {
        char_from_room( mob );
        char_to_room( mob, location );
    }

    char_to_room( ch, location );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING )
        set_position( ch, POS_STANDING );
    if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) && !IS_NPC( ch ) ) {
        if ( ch->level == 108 && !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
            act( AT_CYAN,
                 "A raven flies in suddenly.\r\n\r\n\r\nA raven appears to be studying you.\r\n\r\n&YThe sounds of bones popping fills your ears, as a raven suddenly morphs into a humanoid shape!!\r\n&c$n is standing before you!&D",
                 ch, NULL, NULL, TO_ROOM );
        }
        else if ( !xIS_SET( ch->act, PLR_WIZINVIS ) && ch->level < 108 ) {
            if ( !VLD_STR( ch->pcdata->bamfin ) )
                act( AT_IMMORT,
                     "&C[OoC]: $n please &Rvote&C for us at &Yhttp://6dragons.org/bin/vote.html &Cthis message suddenly &REXPLODES! &cand $n is standing here!&D",
                     ch, NULL, NULL, TO_ROOM );
            else
                act( AT_IMMORT, ch->pcdata->bamfin, ch, NULL, NULL, TO_ROOM );
        }
    }
    if ( IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) && !IS_NPC( ch ) ) {
        act( AT_IMMORT, "&G$n buildwalks to a new room.", ch, NULL, NULL, TO_ROOM );
    }
    else {
        send_to_char( "\r\n&YYou bend your will towards a new destination.\r\n\r\n", ch );
    }
    do_look( ch, ( char * ) "auto" );
    if ( ch->in_room == in_room )
        return;
    for ( fch = in_room->first_person; fch; fch = fch_next ) {
        fch_next = fch->next_in_room;
        if ( fch->master == ch && IS_IMMORTAL( fch ) ) {
            if ( !xIS_SET( fch->act, PLR_BOAT ) ) {
                act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
            }
            do_goto( fch, argument );
        }
        else if ( IS_NPC( fch ) && fch->master == ch ) {
            char_from_room( fch );
            char_to_room( fch, location );
            if ( fch->on ) {
                fch->on = NULL;
                set_position( fch, POS_STANDING );
            }
            if ( fch->position != POS_STANDING )
                set_position( fch, POS_STANDING );
        }
    }
    return;
}

void do_mset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            buf[MSL],
                            outbuf[MSL];
    int                     num,
                            size,
                            plus,
                            v2,
                            value,
                            minattr,
                            maxattr,
                            type;
    char                    char1,
                            char2;
    CHAR_DATA              *victim;
    bool                    lockvictim;
    char                   *origarg = argument;
    char                    string[MAX_STRING_LENGTH];

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't mset\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;
        case SUB_MOB_DESC:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Vladaar.\r\n", ch );
                bug( "do_mset: sub_mob_desc: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            victim = ( CHAR_DATA * ) ch->dest_buf;
            if ( char_died( victim ) ) {
                send_to_char( "Your victim died!\r\n", ch );
                stop_editing( ch );
                return;
            }
            STRFREE( victim->description );
            victim->description = copy_buffer( ch );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->description );
                victim->pIndexData->description = QUICKLINK( victim->description );
            }
            stop_editing( ch );
            ch->substate = ch->tempnum;
            return;
    }

    victim = NULL;
    lockvictim = FALSE;
    smash_tilde( argument );

    if ( ch->substate == SUB_REPEATCMD ) {
        victim = ( CHAR_DATA * ) ch->dest_buf;

        if ( !victim ) {
            send_to_char( "Your victim died!\r\n", ch );
            argument = ( char * ) "done";
        }

        if ( argument[0] == '\0' || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) ) {
            if ( victim )
                do_mstat( ch, victim->name );
            else
                send_to_char( "No victim selected.  Type '?' for help.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) ) {
            if ( ch->dest_buf )
                RelDestroy( relMSET_ON, ch, ch->dest_buf );
            send_to_char( "Mset mode off.\r\n", ch );
            ch->substate = SUB_NONE;
            ch->dest_buf = NULL;
            if ( ch->pcdata && ch->pcdata->subprompt ) {
                STRFREE( ch->pcdata->subprompt );
                ch->pcdata->subprompt = NULL;
            }
            return;
        }
    }
    if ( victim ) {
        lockvictim = TRUE;
        mudstrlcpy( arg1, victim->name, MIL );
        argument = one_argument( argument, arg2 );
        mudstrlcpy( arg3, argument, MIL );
    }
    else {
        lockvictim = FALSE;
        argument = one_argument( argument, arg1 );
        argument = one_argument( argument, arg2 );
        mudstrlcpy( arg3, argument, MIL );
    }

    if ( !str_cmp( arg1, "on" ) ) {
        send_to_char( "Syntax: mset <victim|vnum> on.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' || ( arg2[0] == '\0' && ch->substate != SUB_REPEATCMD )
         || !str_cmp( arg1, "?" ) ) {
        if ( ch->substate == SUB_REPEATCMD ) {
            if ( victim )
                send_to_char( "Syntax: <field>  <value>\r\n", ch );
            else
                send_to_char( "Syntax: <victim> <field>  <value>\r\n", ch );
        }
        else
            send_to_char( "Syntax: mset <victim> <field>  <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  str int wis dex con cha lck sex class hp mana move practice\r\n", ch );
        send_to_char( "  align race carry_weight  hitroll damroll armor affected\r\n", ch );
        send_to_char( "  level thirst drunk full blood flags color (see help mobcolor)\r\n", ch );
        send_to_char( "  pos defpos part (see help BODYPARTS) sav1 sav2 sav4 sav4 sav5\r\n", ch );
        send_to_char( "  (see help SAVINGTHROWS) resistant immune susceptible (see help RIS)\r\n",
                      ch );
        send_to_char( "  attack defense numattacks holdbreath speaking speaks (see LANGUAGES)\r\n",
                      ch );
        send_to_char( "  name short long description title spec clan council htown city auth\r\n",
                      ch );
        send_to_char
            ( "  For mob corpses sliced open objects found : slicevnum (see help slice)\r\n", ch );
        for ( type = 1; type < MAX_CURR_TYPE; type++ )
            ch_printf( ch, "  %s", curr_types[type] );
        send_to_char( " tradeclass tradelevel height weight", ch );
        send_to_char( "\r\n\r\n", ch );
        send_to_char( "Multiclass players only:\r\n", ch );
        send_to_char( "  firstlevel secondlevel thirdlevel secondclass thirdclass\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "For editing index/prototype mobiles:\r\n", ch );
        send_to_char
            ( "  hitnumdie hitsizedie hitplus (hit points) damnumdie damsizedie damplus (damage roll)\r\n",
              ch );
        send_to_char( "Monk Players Only: focus\r\n", ch );
        if ( ch->level == ( MAX_LEVEL ) )
            send_to_char( "PLAYER ONLY: puppet xp dexp faith\r\n", ch );
        return;
    }

    if ( !victim && get_trust( ch ) < LEVEL_IMMORTAL ) {
        if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
    }
    else if ( !victim ) {
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "No one like that in all the realms.\r\n", ch );
            return;
        }
    }

    if ( !IS_NPC( victim ) && IS_IMMORTAL( victim ) && get_trust( victim ) > get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to mset you against your wishes.", ch, NULL, victim, TO_VICT );
        ch->dest_buf = NULL;
        return;
    }
    if ( get_trust( ch ) < LEVEL_AJ_SGT && IS_NPC( victim )
         && xIS_SET( victim->act, ACT_STATSHIELD ) ) {
        send_to_char( "You can't do that!\r\n", ch );
        ch->dest_buf = NULL;
        return;
    }
    if ( lockvictim )
        ch->dest_buf = victim;

    if ( IS_NPC( victim ) || IS_IMMORTAL( victim ) ) {
        minattr = 1;
        maxattr = 25;
    }
    else {
        minattr = 3;
        maxattr = 18;
    }

    if ( !str_cmp( arg2, "on" ) ) {
        CHECK_SUBRESTRICTED( ch );
        ch_printf( ch, "Mset mode on. (Editing %s).\r\n", victim->name );
        ch->substate = SUB_REPEATCMD;
        ch->dest_buf = victim;
        if ( ch->pcdata ) {
            if ( ch->pcdata->subprompt )
                STRFREE( ch->pcdata->subprompt );
            if ( IS_NPC( victim ) )
                snprintf( buf, MSL, "<&CMset &W#%d&w> %%i", victim->pIndexData->vnum );
            else
                snprintf( buf, MSL, "<&CMset &W%s&w> %%i", victim->name );
            ch->pcdata->subprompt = STRALLOC( buf );
        }
        RelCreate( relMSET_ON, ch, victim );
        return;
    }
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;

    if ( atoi( arg3 ) < -1 && value == -1 )
        value = atoi( arg3 );

    if ( !str_cmp( arg2, "xp" ) ) {
        if ( ch->level < MAX_LEVEL )
            return;

        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) )
            return;

        if ( value < 0 || value > 2000000000 ) {
            send_to_char( "Outside of valid experience range 0 - 2bil.\r\n", ch );
            return;
        }

        victim->exp = ( value );
        send_to_char( "Done.\r\n", ch );
        return;

    }

    if ( !str_cmp( arg2, "dexp" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) )
            return;

        if ( value < 0 || value > 2000000000 ) {
            send_to_char( "Outside of valid dexp range 0 - 2bil.\r\n", ch );
            return;
        }

        victim->pcdata->double_exp_timer = ( value );
        send_to_char( "Done.\r\n", ch );
        ch_printf( ch, "%s now has %d rounds of double exp.\r\n", victim->name,
                   victim->pcdata->double_exp_timer / 100 );
        return;
    }

    if ( !str_cmp( arg2, "puppet" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) )
            return;

        if ( IS_PUPPET( victim ) ) {
            REMOVE_BIT( victim->pcdata->flags, PCFLAG_PUPPET );
            send_to_char( "Okay, they're no longer a puppet.\r\n", ch );
        }
        else {
            SET_BIT( victim->pcdata->flags, PCFLAG_PUPPET );
            send_to_char( "They are now a puppet.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "str" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs str.\r\n", ch );
            return;
        }

        if ( value < minattr || value > 25 ) {
            ch_printf( ch, "Strength range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_str = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_str = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    // Faith support.. -Taon
    if ( !str_cmp( arg2, "faith" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) ) {
            send_to_char( "You can only set this on players.\r\n", ch );
            return;
        }
        if ( value > 100 || value < 0 ) {
            send_to_char( "Invalid entry, try again...\r\n", ch );
            return;
        }
        ch->faith = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    // Focus support.. -Taon
    if ( !str_cmp( arg2, "focus" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) ) {
            send_to_char( "You can only set this on players.\r\n", ch );
            return;
        }
        if ( !IS_IMMORTAL( victim ) && victim->Class != CLASS_MONK ) {
            ch_printf( ch, "%s isn't a monk, only monks have use for focus!\r\n", victim->name );
            return;
        }
        if ( value > 50 || value < 0 ) {
            send_to_char( "Invalid value, set between 0 and 50.\r\n", ch );
            return;
        }
        ch->focus_level = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "holdbreath" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "You can only set this on players.\r\n", ch );
            return;
        }

        if ( value < 0 || value > max_holdbreath( ch ) ) {
            ch_printf( ch, "The highest you can set their breath to is %d.\r\n",
                       max_holdbreath( ch ) );
            return;
        }

        ch->pcdata->holdbreath = ( value );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "int" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs int.\r\n", ch );
            return;
        }

        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Intelligence range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_int = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_int = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "wis" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs wis.\r\n", ch );
            return;
        }

        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Wisdom range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_wis = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_wis = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "dex" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs dex.\r\n", ch );
            return;
        }

        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Dexterity range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_dex = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_dex = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "con" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs con.\r\n", ch );
            return;
        }

        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Constitution range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_con = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_con = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "slicevnum" ) ) {
        OBJ_INDEX_DATA         *pObjIndex;

        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) ) {
            send_to_char( "You may only mset mobs slicevnum.\r\n", ch );
            return;
        }

        if ( ( pObjIndex = get_obj_index( value ) ) == NULL ) {
            send_to_char( "No object has that vnum.\r\n", ch );
            return;
        }

        victim->slicevnum = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->slicevnum = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "cha" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( !IS_NPC( victim ) && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char( "You may only mset mobs cha.\r\n", ch );
            return;
        }

        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Charisma range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_cha = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_cha = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hours" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( !is_number( arg3 ) ) {
            send_to_char( "Value must be numeric.\r\n", ch );
            return;
        }

        value = atoi( arg3 );

        if ( value < 0 || value > 999 ) {
            send_to_char( "Value must be betwen 0 and 999.\r\n", ch );
            return;
        }

        value *= 3600;
        victim->played = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "lck" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < minattr || value > maxattr ) {
            ch_printf( ch, "Luck range is %d to %d.\r\n", minattr, maxattr );
            return;
        }
        victim->perm_lck = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->perm_lck = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "height" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( value < 6 || value > 4000 ) {
            ch_printf( ch, "Height range is 6 to 4000.\r\n" );
            return;
        }
        victim->height = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->height = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "carry_weight" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 2000 ) {
            ch_printf( ch,
                       "Carry range is 0 to 2000. This is not the MAXIMUM that a player can carry, just the weight they are currently carrying.\r\n" );
            return;
        }
        victim->carry_weight = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->weight = value;
        send_to_char( "Done.\r\n", ch );
        return;

    }

    if ( !str_cmp( arg2, "weight" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( value < 1 || value > 4000 ) {
            ch_printf( ch, "Weight range is 1 to 4000.\r\n" );
            return;
        }
        victim->weight = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->weight = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sav1" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -30 || value > 30 ) {
            send_to_char( "Saving throw range is -30 to 30.\r\n", ch );
            return;
        }
        victim->saving_poison_death = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->saving_poison_death = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sav2" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -30 || value > 30 ) {
            send_to_char( "Saving throw range is -30 to 30.\r\n", ch );
            return;
        }
        victim->saving_wand = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->saving_wand = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sav3" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -30 || value > 30 ) {
            send_to_char( "Saving throw range is -30 to 30.\r\n", ch );
            return;
        }
        victim->saving_para_petri = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->saving_para_petri = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sav4" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -30 || value > 30 ) {
            send_to_char( "Saving throw range is -30 to 30.\r\n", ch );
            return;
        }
        victim->saving_breath = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->saving_breath = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sav5" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -30 || value > 30 ) {
            send_to_char( "Saving throw range is -30 to 30.\r\n", ch );
            return;
        }
        victim->saving_spell_staff = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->saving_spell_staff = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sex" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 2 ) {
            send_to_char( "Sex range is 0 to 2.\r\n", ch );
            return;
        }
        victim->sex = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->sex = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "class" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            if ( value >= MAX_NPC_CLASS || value < 0 ) {
                ch_printf( ch, "NPC Class range is 0 to %d.\n", MAX_NPC_CLASS - 1 );
                return;
            }
            victim->Class = value;
            if ( xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->Class = value;
            send_to_char( "Done.\r\n", ch );
            return;
        }

        if ( value < 0 || value >= MAX_CLASS ) {
            ch_printf( ch, "Class range is 0 to %d.\n", MAX_CLASS - 1 );
            return;
        }
        victim->Class = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "secondclass" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( value < -1 || value >= MAX_CLASS - 1 ) {
            ch_printf( ch, "Class range is -1 to %d, -1 removes secondclass.\n", MAX_CLASS );
            return;
        }
        victim->secondclass = value;
        if ( value == -1 )
            victim->thirdclass = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "tradeclass" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs cannot have a tradeclass.\r\n", ch );
            return;
        }

        if ( value < -1 || value >= 23 ) {
            ch_printf( ch, "Tradeclass range is 20 to 23, -1 removes tradeclass.\n" );
            return;
        }
        victim->pcdata->tradeclass = value;
        if ( value == -1 )
            victim->pcdata->tradeclass = value;
        send_to_char( "Tradeclass is set.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "tradelevel" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs cannot have a trade level.\r\n", ch );
            return;
        }

        if ( value < 0 || value > 20 ) {
            ch_printf( ch, "Tradelevel range is 1 to 20.\n" );
            return;
        }
        victim->pcdata->tradelevel = value;
        if ( value == 0 )
            victim->pcdata->tradelevel = value;
        send_to_char( "Tradelevel is set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "extlevel" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs cannot have a extlevel.\r\n", ch );
            return;
        }
        victim->pcdata->extlevel = value;
        if ( value == 0 )
            victim->pcdata->extlevel = value;
        send_to_char( "Extreme level is set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "craftpoints" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs cannot have craftpoints.\r\n", ch );
            return;
        }

        if ( value < 0 || value >= 50000 ) {
            ch_printf( ch, "The range is 0 to 50000.\n" );
            return;
        }
        victim->pcdata->craftpoints = value;
        if ( value == 0 )
            victim->pcdata->craftpoints = value;
        send_to_char( "Craftpoints is set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "thirdclass" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs cannot have more then one class.\r\n", ch );
            return;
        }

        if ( value < -1 || value >= MAX_CLASS - 1 ) {
            ch_printf( ch, "Class range is -1 to %d, -1 removes thirdclass.\n", MAX_CLASS );
            return;
        }
        victim->thirdclass = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "firstlevel" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on mobs.\r\n", ch );
            return;
        }

        if ( value < 1 || value >= ( MAX_LEVEL - 10 ) ) {
            ch_printf( ch, "Level range is 1 to %d.\n", MAX_LEVEL - 10 );
            return;
        }
        victim->firstlevel = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "secondlevel" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on mobs.\r\n", ch );
            return;
        }

        if ( value < 1 || value > ( MAX_LEVEL - 8 ) ) {
            ch_printf( ch, "Level range is 1 to %d.\n", MAX_LEVEL - 8 );
            return;
        }
        victim->secondlevel = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "thirdlevel" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on mobs.\r\n", ch );
            return;
        }

        if ( value < 1 || value > ( MAX_LEVEL - 8 ) ) {
            ch_printf( ch, "Level range is 1 to %d.\n", MAX_LEVEL - 8 );
            return;
        }
        victim->thirdlevel = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "race" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) )
            value = get_npc_race( arg3 );
        else
            value = get_pc_race( arg3 );

        if ( value < 0 )
            value = atoi( arg3 );
        if ( !IS_NPC( victim ) && ( value < 0 || value >= MAX_RACE ) ) {
            ch_printf( ch, "Race range is 0 to %d.\n", MAX_RACE - 1 );
            return;
        }
        if ( IS_NPC( victim ) && ( value < 0 || value >= MAX_NPC_RACE ) ) {
            ch_printf( ch, "Race range is 0 to %d.\n", MAX_NPC_RACE - 1 );
            return;
        }
        victim->race = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->race = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "armor" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) && ch->level < 106 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You cannot readjust armor, contact the head of Area Architect for more information.\r\n",
                  ch );
            return;
        }
        if ( value < -1500 || value > 500 ) {
            send_to_char( "AC range is -1500 to 500.\r\n", ch );
            return;
        }
        victim->armor = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->ac = value;
        send_to_char( "New armor class setting applied.\r\n", ch );
        return;
    }

    // Modified to adjust ac, hps, numattacks by level, just the start. -Taon
    if ( !str_cmp( arg2, "level" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Not on PC's.\r\n", ch );
            return;
        }

        if ( value < 0 || value > MAX_LEVEL ) {
            ch_printf( ch, "Level range is 0 to %d.\r\n", MAX_LEVEL );
            return;
        }

        victim->level = value;

        if ( !xIS_SET( victim->act, ACT_OVERRIDE ) ) {
            send_to_char
                ( "Now establishing Hitpoints, ArmorClass, Hitroll/Damroll and NumAttacks.\r\n",
                  ch );
            send_to_char
                ( "You need only set the race/class, sex, short/long desc, attacks, bodyparts, affected, and attributes.\r\n",
                  ch );
            send_to_char
                ( "If you have a quest mob, or a reason why it needs to be outside autoset, see a Head of Council.\r\n",
                  ch );
            victim->numattacks = set_num_attacks( value );
            victim->armor = set_armor_class( value );
            victim->max_hit = set_hp( value );
            victim->hitplus = set_hp( value );
            restore_char( victim );
            victim->hitroll = set_hitroll( value );
            victim->damroll = set_damroll( value );
        }

        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE )
             && !xIS_SET( victim->act, ACT_OVERRIDE ) )
            victim->pIndexData->hitplus = set_hp( value );

        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->level = value;

        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "numattacks" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Not on PC's.\r\n", ch );
            return;
        }
        if ( ch->level < 106 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                  ch );
            return;
        }
        if ( value < 0 || value > 20 ) {
            send_to_char( "Attacks range is 0 to 20.\r\n", ch );
            return;
        }
        victim->numattacks = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->numattacks = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( ( type = get_currency_type( arg2 ) ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        GET_MONEY( victim, type ) = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            GET_MONEY( victim->pIndexData, type ) = value;
        ch_printf( ch, "%s now has %d %s.\r\n", PERS( victim, ch ), GET_MONEY( victim, type ),
                   cap_curr_types[type] );

        return;
    }

    if ( !str_cmp( arg2, "hitroll" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) && ch->level < 106 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You can't modify hitroll, contact the head of\r\nArea Architect in order to have this value changed.",
                  ch );
            return;
        }
        victim->hitroll = URANGE( 0, value, 200 );
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->hitroll = victim->hitroll;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "damroll" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) && ch->level < 106 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You can't modify damroll, contact the head of\r\nArea Architect in order to have this value changed.",
                  ch );
            return;
        }
        victim->damroll = URANGE( 0, value, 200 );
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->damroll = victim->damroll;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hp" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( IS_NPC( victim ) && ch->level < 106 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You can't modify hitpoints, contact the head of\r\nArea Architect in order to have this value changed.",
                  ch );
            return;
        }
        if ( value < 1 || value > 300000 ) {
            send_to_char( "Hp range is 1 to 300,000 hit points.\r\n", ch );
            return;
        }

        victim->max_hit = value;

        if ( IS_NPC( victim ) ) {
            victim->hitplus = value;
            victim->pIndexData->hitplus = value;
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "mana" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 300000 ) {
            send_to_char( "Mana range is 0 to 300,000 mana points.\r\n", ch );
            return;
        }
        victim->max_mana = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "move" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 300000 ) {
            send_to_char( "Move range is 0 to 300,000 move points.\r\n", ch );
            return;
        }
        victim->max_move = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "practice" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 100 ) {
            send_to_char( "Practice range is 0 to 100 sessions.\r\n", ch );
            return;
        }
        victim->practice = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "align" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < -1000 || value > 1000 ) {
            send_to_char( "Alignment range is -1000 to 1000.\r\n", ch );
            return;
        }
        victim->alignment = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->alignment = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "password" ) ) {
        char                   *pwdnew;
        char                   *p;

        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) ) {
            send_to_char( "Mobs don't have passwords.\r\n", ch );
            return;
        }

        if ( strlen( arg3 ) < 5 ) {
            send_to_char( "New password must be at least five characters long.\r\n", ch );
            return;
        }

        if ( arg3[0] == '!' ) {
            send_to_char( "New password cannot begin with the '!' character.", ch );
            return;
        }

        /*
         * No tilde allowed because of player file format.
         */
        pwdnew = crypt( arg3, ch->name );
        for ( p = pwdnew; *p != '\0'; p++ ) {
            if ( *p == '~' ) {
                send_to_char( "New password not acceptable, try again.\r\n", ch );
                return;
            }
        }

        STRFREE( victim->pcdata->pwd );
        victim->pcdata->pwd = STRALLOC( pwdnew );
        if ( xIS_SET( sysdata.save_flags, SV_PASSCHG ) )
            save_char_obj( victim );
        send_to_char( "Ok.\r\n", ch );
        ch_printf( victim, "Your password has been changed by %s.\r\n", ch->name );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "rank" ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }
        smash_tilde( argument );
        STRFREE( victim->pcdata->rank );
        if ( VLD_STR( argument ) && str_cmp( argument, "none" ) )
            victim->pcdata->rank = STRALLOC( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "auth" ) ) {
        if ( IS_NPC( ch ) || IS_NPC( victim ) ) {
            send_to_char( "A mob cannot use this command, nor can it be used on mobs.\r\n", ch );
            return;
        }
        if ( victim->pcdata->authed_by )
            STRFREE( victim->pcdata->authed_by );
        victim->pcdata->authed_by = QUICKLINK( ch->name );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "mentalstate" ) ) {
        if ( value < -100 || value > 100 ) {
            send_to_char( "Value must be in range -100 to +100.\r\n", ch );
            return;
        }
        victim->mental_state = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "emotion" ) ) {
        if ( value < -100 || value > 100 ) {
            send_to_char( "Value must be in range -100 to +100.\r\n", ch );
            return;
        }
        victim->emotional_state = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "thirst" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( value < 0 || value > 100 ) {
            send_to_char( "Thirst range is 0 to 100.\r\n", ch );
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "drunk" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( value < 0 || value > 100 ) {
            send_to_char( "Drunk range is 0 to 100.\r\n", ch );
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "full" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( value < 0 || value > 100 ) {
            send_to_char( "Full range is 0 to 100.\r\n", ch );
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "blood" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( value < 0 || value > 300000 ) {
            send_to_char( "Blood application range is 0 to 300,000 blood points.\r\n", ch );
            return;
        }
        victim->max_blood = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Not on PC's.\r\n", ch );
            return;
        }
        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "You can't set the name to nothing.\r\n", ch );
            return;
        }
        string_stripcolor( arg3, string );

        if ( !VLD_STR( string ) ) {
            send_to_char( "You can't set the name to nothing.\r\n", ch );
            return;
        }

        if ( VLD_STR( victim->name ) )
            STRFREE( victim->name );

        victim->name = STRALLOC( string );

        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
            if ( VLD_STR( victim->pIndexData->player_name ) )
                STRFREE( victim->pIndexData->player_name );
            victim->pIndexData->player_name = QUICKLINK( victim->name );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "minsnoop" ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }
        if ( victim->pcdata ) {
            victim->pcdata->min_snoop = value;
            send_to_char( "Done.\r\n", ch );
            return;
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "clan" ) ) {
        CLAN_DATA              *clan;

        if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( !VLD_STR( arg3 ) ) {
            /*
             * Crash bug fix, oops guess I should have caught this one :)
             * * But it was early in the morning :P --Shaddai 
             */
            if ( victim->pcdata->clan == NULL )
                return;
            /*
             * Added a check on immortals so immortals don't take up
             * * any membership space. --Shaddai
             */
            if ( !IS_IMMORTAL( victim ) ) {
                remove_roster( victim->pcdata->clan, victim->name );
                --victim->pcdata->clan->members;
                save_clan( victim->pcdata->clan );
            }
            if ( VLD_STR( victim->pcdata->clan_name ) )
                STRFREE( victim->pcdata->clan_name );
            victim->pcdata->clan = NULL;
            return;
        }
        clan = get_clan( arg3 );
        if ( !clan ) {
            send_to_char( "No such clan.\r\n", ch );
            return;
        }
        if ( victim->pcdata->clan != NULL && !IS_IMMORTAL( victim ) ) {
            remove_roster( victim->pcdata->clan, victim->name );
            --victim->pcdata->clan->members;
            save_clan( victim->pcdata->clan );
        }
        if ( VLD_STR( victim->pcdata->clan_name ) )
            STRFREE( victim->pcdata->clan_name );
        victim->pcdata->clan_name = QUICKLINK( clan->name );
        victim->pcdata->clan = clan;

        if ( !IS_IMMORTAL( victim ) ) {
            ++victim->pcdata->clan->members;
            add_roster( victim->pcdata->clan, victim->name,
                        capitalize( race_table[victim->race]->race_name ), victim->level,
                        victim->pcdata->pkills, victim->pcdata->mkills, victim->pcdata->mdeaths,
                        victim->pcdata->tradeclass, victim->pcdata->tradelevel );
            save_clan( victim->pcdata->clan );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "council" ) ) {
        COUNCIL_DATA           *council;

        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( !VLD_STR( arg3 ) ) {
            if ( !IS_IMMORTAL( victim ) && victim->pcdata->council ) {
                if ( victim->pcdata->council->members > 1 )
                    --victim->pcdata->council->members;
                save_council( victim->pcdata->council );
            }
            if ( VLD_STR( victim->pcdata->council_name ) )
                STRFREE( victim->pcdata->council_name );
            victim->pcdata->council = NULL;
            send_to_char( "Removed from council.\r\n", ch );
            return;
        }

        council = get_council( arg3 );
        if ( !council ) {
            send_to_char( "No such council.\r\n", ch );
            return;
        }
        /*
         * Update the members list 
         */
        if ( !IS_IMMORTAL( victim ) ) {
            ++council->members;
            save_council( council );
        }
        if ( VLD_STR( victim->pcdata->council_name ) )
            STRFREE( victim->pcdata->council_name );
        victim->pcdata->council_name = QUICKLINK( council->name );
        victim->pcdata->council = council;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "htown" ) ) {
        HTOWN_DATA             *htown;

        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( !VLD_STR( arg3 ) ) {
            if ( !IS_IMMORTAL( victim ) && victim->pcdata->htown ) {
                if ( victim->pcdata->htown->members > 1 )
                    --victim->pcdata->htown->members;
                save_htown( victim->pcdata->htown );
            }
            if ( VLD_STR( victim->pcdata->htown_name ) )
                STRFREE( victim->pcdata->htown_name );
            victim->pcdata->htown = NULL;
            send_to_char( "Removed from htown.\r\n", ch );
            return;
        }

        htown = get_htown( arg3 );
        if ( !htown ) {
            send_to_char( "No such hometown.\r\n", ch );
            return;
        }
        if ( !IS_IMMORTAL( victim ) ) {
            ++victim->pcdata->htown->members;
            save_htown( victim->pcdata->htown );
        }
        if ( VLD_STR( victim->pcdata->htown_name ) )
            STRFREE( victim->pcdata->htown_name );
        victim->pcdata->htown_name = QUICKLINK( htown->name );
        victim->pcdata->htown = htown;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "city" ) ) {
        CITY_DATA              *city,
                               *ocity;

        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( ( ocity = victim->pcdata->city ) ) {
            if ( VLD_STR( victim->pcdata->city_name ) )
                STRFREE( victim->pcdata->city_name );
            victim->pcdata->city = NULL;
            remove_rollcall( ocity, victim->name );
            save_char_obj( victim );
            ch_printf( ch, "They were removed from %s.\r\n", ocity->name );
        }

        if ( !VLD_STR( arg3 ) )
            return;

        city = get_city( arg3 );
        if ( !city ) {
            send_to_char( "No such city.\r\n", ch );
            return;
        }
        if ( VLD_STR( victim->pcdata->city_name ) )
            STRFREE( victim->pcdata->city_name );
        victim->pcdata->city_name = QUICKLINK( city->name );
        victim->pcdata->city = city;
        add_rollcall( city, victim->name, victim->Class, victim->level, victim->pcdata->mkills,
                      victim->pcdata->mdeaths );
        save_char_obj( victim );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "short" ) ) {
        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Short can't be set to nothing.\r\n", ch );
            return;
        }
        string_stripcolor( arg3, string );
        if ( !VLD_STR( string ) ) {
            send_to_char( "Short can't be set to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( victim->short_descr ) )
            STRFREE( victim->short_descr );
        victim->short_descr = STRALLOC( string );

        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
            if ( VLD_STR( victim->pIndexData->short_descr ) )
                STRFREE( victim->pIndexData->short_descr );
            victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "color" ) ) {
        if ( value < 0 || value > 14 ) {
            send_to_char( "Color range is 0 to 14 color types.\r\n", ch );
            return;
        }
        victim->color = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
            victim->pIndexData->color = value;
        }
        send_to_char( "mob color set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "long" ) ) {
        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Can't set long to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( victim->long_descr ) )
            STRFREE( victim->long_descr );
        string_stripcolor( arg3, string );
        if ( !VLD_STR( string ) ) {
            send_to_char( "Can't set long to nothing.\r\n", ch );
            return;
        }
        mudstrlcpy( buf, string, MSL );
        mudstrlcat( buf, "\r\n", MSL );
        victim->long_descr = STRALLOC( buf );
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
            if ( VLD_STR( victim->pIndexData->long_descr ) )
                STRFREE( victim->pIndexData->long_descr );
            victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "description" ) ) {
        if ( VLD_STR( arg3 ) ) {
            if ( VLD_STR( victim->description ) )
                STRFREE( victim->description );
            victim->description = STRALLOC( arg3 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->description );
                victim->pIndexData->description = QUICKLINK( victim->description );
            }
            send_to_char( "Done.\r\n", ch );
            return;
        }
        CHECK_SUBRESTRICTED( ch );
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_MOB_DESC;
        ch->dest_buf = victim;
        start_editing( ch, victim->description );
        if ( IS_NPC( victim ) )
            editor_desc_printf( ch, "Description of mob, vnum %ld (%s).", victim->pIndexData->vnum,
                                victim->name );
        else
            editor_desc_printf( ch, "Description of player %s.", capitalize( victim->name ) );
        return;
    }

    if ( !str_cmp( arg2, "title" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }
        set_title( victim, arg3 );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "spec" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Not on PC's.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg3, "none" ) ) {
            victim->spec_fun = NULL;
            send_to_char( "Special function removed.\r\n", ch );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->spec_fun = victim->spec_fun;
            return;
        }

        if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 ) {
            send_to_char( "No such spec fun.\r\n", ch );
            return;
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->spec_fun = victim->spec_fun;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        bool                    pcflag;

        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "You can only modify a mobile's flags.\r\n", ch );
            return;
        }

        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> flags <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            pcflag = FALSE;
            argument = one_argument( argument, arg3 );
            value = IS_NPC( victim ) ? get_actflag( arg3 ) : get_plrflag( arg3 );

            if ( !IS_NPC( victim ) && ( value < 0 || value > MAX_BITS ) ) {
                pcflag = TRUE;
                value = get_pcflag( arg3 );
            }
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else {
                if ( IS_NPC( victim ) && value == ACT_PROTOTYPE && get_trust( ch ) < LEVEL_AJ_SGT
                     && !is_name( "protoflag", ch->pcdata->bestowments ) )
                    send_to_char( "You cannot change the prototype flag.\r\n", ch );
                else if ( value == ACT_IS_NPC )
                    send_to_char( "If that could be changed, it would cause many problems.\r\n",
                                  ch );
                else {
                    if ( pcflag )
                        TOGGLE_BIT( victim->pcdata->flags, 1 << value );
                    else {
                        xTOGGLE_BIT( victim->act, value );
                        /*
                         * NPC check added by Gorog 
                         */
                        if ( IS_NPC( victim ) && value == ACT_PROTOTYPE )
                            victim->pIndexData->act = victim->act;
                    }
                }
            }
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->act = victim->act;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "affected" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's flags.\r\n", ch );
            return;
        }

        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> affected <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_aflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( victim->affected_by, value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->affected_by = victim->affected_by;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    /*
     * save some more finger-leather for setting RIS stuff
     */
    if ( !str_cmp( arg2, "r" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "i" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "s" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "ri" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "rs" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "is" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "ris" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's ris.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "resistant" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's resistancies.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> resistant <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( victim->resistant, 1 << value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->resistant = victim->resistant;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "immune" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's immunities.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> immune <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( victim->immune, 1 << value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->immune = victim->immune;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "susceptible" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's susceptibilities.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> susceptible <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( victim->susceptible, 1 << value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->susceptible = victim->susceptible;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "part" ) ) {
        if ( !IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only modify a mobile's parts.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> part <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_partflag( arg3 );
            if ( value < 0 || value >= MAX_PARTS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( victim->xflags, value );
        }

        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->xflags = victim->xflags;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "attack" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "You can only modify a mobile's attacks.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> attack <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_attackflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( victim->attacks, value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->attacks = victim->attacks;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "defense" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "You can only modify a mobile's defenses.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> defense <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_defenseflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( victim->defenses, value );
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->defenses = victim->defenses;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "pos" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > POS_STANDING ) {
            ch_printf( ch, "Position range is 0 to %d.\r\n", POS_STANDING );
            return;
        }
        set_position( victim, value );
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->position = victim->position;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "defpos" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > POS_STANDING ) {
            ch_printf( ch, "Position range is 0 to %d.\r\n", POS_STANDING );
            return;
        }
        victim->defposition = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->defposition = victim->defposition;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    /*
     * save some finger-leather
     */
    if ( !str_cmp( arg2, "hitdie" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        sscanf( arg3, "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
        snprintf( outbuf, MSL, "%s hitnumdie %d", arg1, num );
        do_mset( ch, outbuf );

        snprintf( outbuf, MSL, "%s hitsizedie %d", arg1, size );
        do_mset( ch, outbuf );

        snprintf( outbuf, MSL, "%s hitplus %d", arg1, plus );
        do_mset( ch, outbuf );
        return;
    }
    /*
     * save some more finger-leather
     */
    if ( !str_cmp( arg2, "damdie" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;

        sscanf( arg3, "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
        snprintf( outbuf, MSL, "%s damnumdie %d", arg1, num );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s damsizedie %d", arg1, size );
        do_mset( ch, outbuf );
        snprintf( outbuf, MSL, "%s damplus %d", arg1, plus );
        do_mset( ch, outbuf );
        return;
    }

/*  if(!str_cmp(arg2, "hitnumdie"))
  {
    if(!IS_NPC(victim))
    {
      send_to_char("Mobiles only.\r\n", ch);
      return;
    }
    if(!can_mmodify(ch, victim))
      return;
    if(value < 0 || value > 32767)
    {
      send_to_char("Number of hitpoint dice range is 0 to 32,767.\r\n", ch);
      return;
    }
    if(IS_NPC(victim) && xIS_SET(victim->act, ACT_PROTOTYPE))
    {
      victim->pIndexData->hitnodice = value;
      victim->max_hit = generate_hp(victim->level, victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice, victim->pIndexData->hitplus);
      send_to_char("Done.\r\n", ch);
    }
    else
      send_to_char("Mobile has to be set to prototype to change this.\r\n", ch);
    return;
  }

  if(!str_cmp(arg2, "hitsizedie"))
  {
    if(!IS_NPC(victim))
    {
      send_to_char("Mobiles only.\r\n", ch);
      return;
    }
    if(!can_mmodify(ch, victim))
      return;
    if(value < 0 || value > 32767)
    {
      send_to_char("Hitpoint dice size range is 0 to 32,767.\r\n", ch);
      return;
    }
    if(IS_NPC(victim) && xIS_SET(victim->act, ACT_PROTOTYPE))
    {
      victim->pIndexData->hitsizedice = value;
      victim->max_hit = generate_hp(victim->level, victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice, victim->pIndexData->hitplus);
      send_to_char("Done.\r\n", ch);
    }
    else
      send_to_char("Mobile has to be set to prototype to change this.\r\n", ch);
    return;
  }

  if(!str_cmp(arg2, "hitplus"))
  {
    if(!IS_NPC(victim))
    {
      send_to_char("Mobiles only.\r\n", ch);
      return;
    }
    if(!can_mmodify(ch, victim))
      return;
    if(value < 0 || value > 32767)
    {
      send_to_char("Hitpoint bonus range is 0 to 32,767.\r\n", ch);
      return;
    }
    if(IS_NPC(victim) && xIS_SET(victim->act, ACT_PROTOTYPE))
    {
      victim->pIndexData->hitplus = value;
      victim->max_hit = generate_hp(victim->level, victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice, victim->pIndexData->hitplus);
      send_to_char("Done.\r\n", ch);
    }
    else
      send_to_char("Mobile has to be prototype to change this.\r\n", ch);
    return;
  }
*/

    if ( !str_cmp( arg2, "damnumdie" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 100 ) {
            send_to_char( "Number of damage dice range is 0 to 100.\r\n", ch );
            return;
        }
        victim->barenumdie = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->damnodice = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "damsizedie" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 100 ) {
            send_to_char( "Damage dice size range is 0 to 100.\r\n", ch );
            return;
        }
        victim->baresizedie = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->damsizedice = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "damplus" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Mobiles only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( value < 0 || value > 1000 ) {
            send_to_char( "Damage bonus range is 0 to 1000.\r\n", ch );
            return;
        }
        victim->damplus = value;
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->damplus = value;
        send_to_char( "Done.\r\n", ch );
        return;

    }

    if ( !str_cmp( arg2, "aloaded" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Player Characters only.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !victim->pcdata->area ) {
            send_to_char( "Player does not have an area assigned to them.\r\n", ch );
            return;
        }
        if ( !IS_SET( victim->pcdata->area->status, AREA_LOADED ) ) {
            SET_BIT( victim->pcdata->area->status, AREA_LOADED );
            send_to_char( "Your area set to LOADED!\r\n", victim );
            if ( ch != victim )
                send_to_char( "Area set to LOADED!\r\n", ch );
            return;
        }
        else {
            REMOVE_BIT( victim->pcdata->area->status, AREA_LOADED );
            send_to_char( "Your area set to NOT-LOADED!\r\n", victim );
            if ( ch != victim )
                send_to_char( "Area set to NON-LOADED!\r\n", ch );
            return;
        }
    }

    if ( !str_cmp( arg2, "pkill" ) ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Player Characters only.\r\n", ch );
            return;
        }

        if ( !can_mmodify( ch, victim ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }

        if ( IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) {
            REMOVE_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
            xSET_BIT( victim->act, PLR_NICE );
            send_to_char( "You are now a NON-PKILL player.\r\n", victim );
            if ( ch != victim )
                send_to_char( "That player is now non-pkill.\r\n", ch );
        }
        else {
            SET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
            xREMOVE_BIT( victim->act, PLR_NICE );
            send_to_char( "You are now a PKILL player.\r\n", victim );
            if ( ch != victim )
                send_to_char( "That player is now pkill.\r\n", ch );
        }
        if ( victim->pcdata->clan && !IS_IMMORTAL( victim ) ) {
            if ( victim->pcdata->clan->members > 1 )
                --victim->pcdata->clan->members;
            save_clan( victim->pcdata->clan );
            STRFREE( victim->pcdata->clan_name );
            victim->pcdata->clan = NULL;
        }
        save_char_obj( victim );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "speaks" ) ) {
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> speaks <language> [language] ...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_langflag( arg3 );
            if ( value == LANG_UNKNOWN )
                ch_printf( ch, "Unknown language: %s\r\n", arg3 );
            else if ( !IS_NPC( victim ) ) {
                if ( !( value &= VALID_LANGS ) ) {
                    ch_printf( ch, "Players may not know %s.\r\n", arg3 );
                    continue;
                }
            }

            v2 = get_langnum( arg3 );
            if ( v2 == -1 )
                ch_printf( ch, "Unknown language: %s\r\n", arg3 );
            else
                TOGGLE_BIT( victim->speaks, 1 << v2 );
        }
        if ( !IS_NPC( victim ) ) {
            REMOVE_BIT( victim->speaks, race_table[victim->race]->language );
            if ( !knows_language( victim, victim->speaking, victim ) )
                victim->speaking = race_table[victim->race]->language;
        }
        else if ( xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->speaks = victim->speaks;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "speaking" ) ) {
        if ( !IS_NPC( victim ) ) {
            send_to_char( "Players must choose the language they speak themselves.\r\n", ch );
            return;
        }
        if ( !can_mmodify( ch, victim ) )
            return;
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: mset <victim> speaking <language> [language]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_langflag( arg3 );
            if ( value == LANG_UNKNOWN )
                ch_printf( ch, "Unknown language: %s\r\n", arg3 );
            else {
                v2 = get_langnum( arg3 );
                if ( v2 == -1 )
                    ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                else
                    TOGGLE_BIT( victim->speaking, 1 << v2 );
            }
        }
        if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            victim->pIndexData->speaking = victim->speaking;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    /*
     * Generate usage message.
     */
    if ( ch->substate == SUB_REPEATCMD ) {
        ch->substate = SUB_RESTRICTED;
        interpret( ch, origarg );
        ch->substate = SUB_REPEATCMD;
        ch->last_cmd = do_mset;
    }
    else
        do_mset( ch, ( char * ) "" );
    return;
}

void do_oset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            buf[MSL],
                            outbuf[MSL];
    OBJ_DATA               *obj,
                           *tmpobj;
    EXTRA_DESCR_DATA       *ed;
    bool                    lockobj;
    char                   *origarg = argument;
    char                    string[MAX_STRING_LENGTH];

    int                     value,
                            tmp;

    set_char_color( AT_PLAIN, ch );
    if ( !ch )
        return;
    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't oset\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }
    switch ( ch->substate ) {
        default:
            break;
        case SUB_OBJ_EXTRA:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Thoric.\r\n", ch );
                bug( "do_oset: sub_obj_extra: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            /*
             * hopefully the object didn't get extracted...
             * * if you're REALLY paranoid, you could always go through
             * * the object and index-object lists, searching through the
             * * extra_descr lists for a matching pointer...
             */
            ed = ( EXTRA_DESCR_DATA * ) ch->dest_buf;
            if ( VLD_STR( ed->description ) )
                STRFREE( ed->description );
            ed->description = copy_buffer( ch );
            tmpobj = ( OBJ_DATA * ) ch->spare_ptr;
            stop_editing( ch );
            ch->dest_buf = tmpobj;
            ch->substate = ch->tempnum;
            return;
        case SUB_OBJ_LONG:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Thoric.\r\n", ch );
                bug( "do_oset: sub_obj_long: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            obj = ( OBJ_DATA * ) ch->dest_buf;
            if ( obj && obj_extracted( obj ) ) {
                send_to_char( "Your object was extracted!\r\n", ch );
                stop_editing( ch );
                return;
            }
            if ( VLD_STR( obj->description ) )
                STRFREE( obj->description );
            obj->description = copy_buffer( ch );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                if ( can_omodify( ch, obj ) ) {
                    if ( VLD_STR( obj->pIndexData->description ) )
                        STRFREE( obj->pIndexData->description );
                    obj->pIndexData->description = QUICKLINK( obj->description );
                }
            }
            tmpobj = ( OBJ_DATA * ) ch->spare_ptr;
            stop_editing( ch );
            ch->substate = ch->tempnum;
            ch->dest_buf = tmpobj;
            return;
    }
    obj = NULL;
    smash_tilde( argument );
    if ( ch->substate == SUB_REPEATCMD ) {
        obj = ( OBJ_DATA * ) ch->dest_buf;
        if ( !obj ) {
            send_to_char( "Your object was extracted!\r\n", ch );
            argument = ( char * ) "done";
        }
        if ( !VLD_STR( argument ) || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) ) {
            if ( obj )
                do_ostat( ch, obj->name );
            else
                send_to_char( "No object selected.  Type '?' for help.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) ) {
            if ( ch->dest_buf )
                RelDestroy( relOSET_ON, ch, ch->dest_buf );
            send_to_char( "Oset mode off.\r\n", ch );
            ch->substate = SUB_NONE;
            ch->dest_buf = NULL;
            if ( ch->pcdata && ch->pcdata->subprompt ) {
                if ( VLD_STR( ch->pcdata->subprompt ) )
                    STRFREE( ch->pcdata->subprompt );
                ch->pcdata->subprompt = NULL;
            }
            return;
        }
    }
    if ( obj ) {
        lockobj = TRUE;
        mudstrlcpy( arg1, obj->name, MIL );
        argument = one_argument( argument, arg2 );
        mudstrlcpy( arg3, argument, MIL );
    }
    else {
        lockobj = FALSE;
        argument = one_argument( argument, arg1 );
        argument = one_argument( argument, arg2 );
        mudstrlcpy( arg3, argument, MIL );
    }
    if ( !str_cmp( arg1, "on" ) ) {
        send_to_char( "Syntax: oset <object|vnum> on.\r\n", ch );
        return;
    }
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !str_cmp( arg1, "?" ) ) {
        if ( ch->substate == SUB_REPEATCMD ) {
            if ( obj )
                send_to_char( "Syntax: <field>  <value>\r\n", ch );
            else
                send_to_char( "Syntax: <object> <field>  <value>\r\n", ch );
        }
        else
            send_to_char( "Syntax: oset <object> <field>  <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  flags wear level weight cost rent timer\r\n", ch );
        send_to_char( "  name short long ed rmed actiondesc type\r\n", ch );
        send_to_char( "  color( see help objectcolor )\r\n", ch );
        send_to_char( "  value0 value1 value2 value3 value4 value5 value6\r\n", ch );
        send_to_char( "  size affect rmaffect layers currtype owner\r\n", ch );
        send_to_char( "For weapons:             For armor:\r\n", ch );
        send_to_char( "  weapontype condition     ac condition\r\n", ch );
        send_to_char( "For scrolls, potions and pills:\r\n", ch );
        send_to_char( "  slevel spell1 spell2 spell3\r\n", ch );
        send_to_char( "For wands and staves:\r\n", ch );
        send_to_char( "  slevel spell maxcharges charges\r\n", ch );
        send_to_char( "For containers:          For levers and switches:\r\n", ch );
        send_to_char( "  cflags key capacity      tflags\r\n", ch );
        return;
    }
    if ( !obj && get_trust( ch ) < LEVEL_IMMORTAL ) {
        if ( ( obj = get_obj_here( ch, arg1 ) ) == NULL ) {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
        }
    }
    else if ( !obj ) {
        if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "There is nothing like that in all the realms.\r\n", ch );
            return;
        }
    }
    if ( lockobj )
        ch->dest_buf = obj;
    else
        ch->dest_buf = NULL;
    separate_obj( obj );
    value = atoi( arg3 );
    if ( !str_cmp( arg2, "on" ) ) {
        ch_printf( ch, "Oset mode on. (Editing '%s' vnum %d).\r\n", obj->name,
                   obj->pIndexData->vnum );
        ch->substate = SUB_REPEATCMD;
        ch->dest_buf = obj;
        if ( ch->pcdata ) {
            if ( ch->pcdata->subprompt )
                STRFREE( ch->pcdata->subprompt );
            snprintf( buf, MSL, "<&COset &W#%d&w> %%i", obj->pIndexData->vnum );
            ch->pcdata->subprompt = STRALLOC( buf );
        }
        RelCreate( relOSET_ON, ch, obj );
        return;
    }
    if ( !str_cmp( arg2, "name" ) ) {
        bool                    proto = FALSE;

        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            proto = TRUE;
        if ( proto && !can_omodify( ch, obj ) )
            return;

        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Can't set name to nothing.\r\n", ch );
            return;
        }
        string_stripcolor( arg3, string );
        if ( !VLD_STR( string ) ) {
            send_to_char( "Can't set name to nothing.\r\n", ch );
            return;
        }

        if ( VLD_STR( obj->name ) )
            STRFREE( obj->name );

        obj->name = STRALLOC( string );

        if ( proto ) {
            if ( VLD_STR( obj->pIndexData->name ) )
                STRFREE( obj->pIndexData->name );
            obj->pIndexData->name = QUICKLINK( obj->name );
        }
        return;
    }
    if ( !str_cmp( arg2, "color" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( value < 0 || value > 14 ) {
            send_to_char( "Color range is 0 to 14 color types.\r\n", ch );
            return;
        }
        obj->color = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->color = value;
        return;
    }
    if ( !str_cmp( arg2, "short" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;

        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Can't set short to nothing.\r\n", ch );
            return;
        }

        string_stripcolor( arg3, string );

        if ( !VLD_STR( string ) ) {
            send_to_char( "Can't set short to nothing.\r\n", ch );
            return;
        }

        if ( VLD_STR( obj->short_descr ) )
            STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( string );

        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && VLD_STR( obj->pIndexData->short_descr ) )
                STRFREE( obj->pIndexData->short_descr );
            obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
        }
        else {
            if ( str_infix( "rename", obj->name ) ) {
                snprintf( buf, MSL, "%s %s", obj->name, "rename" );
                if ( VLD_STR( obj->name ) )
                    STRFREE( obj->name );
                obj->name = STRALLOC( buf );
            }
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "long" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( !VLD_STR( arg3 ) ) {
            CHECK_SUBRESTRICTED( ch );
            if ( ch->substate == SUB_REPEATCMD )
                ch->tempnum = SUB_REPEATCMD;
            else
                ch->tempnum = SUB_NONE;
            if ( lockobj )
                ch->spare_ptr = obj;
            else
                ch->spare_ptr = NULL;
            ch->substate = SUB_OBJ_LONG;
            ch->dest_buf = obj;
            start_editing( ch, obj->description );
            editor_desc_printf( ch, "Object long desc, vnum %ld (%s).", obj->pIndexData->vnum,
                                obj->short_descr );
            return;
        }

        string_stripcolor( arg3, string );

        if ( !VLD_STR( string ) ) {
            send_to_char( "Can't set long to nothing.\r\n", ch );
            return;
        }

        if ( VLD_STR( obj->description ) )
            STRFREE( obj->description );
        obj->description = STRALLOC( string );

        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            if ( VLD_STR( obj->pIndexData->description ) )
                STRFREE( obj->pIndexData->description );
            obj->pIndexData->description = QUICKLINK( obj->description );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }
    // Volk: info cmd
    if ( !str_cmp( arg2, "glory" ) ) {
        send_to_char
            ( "You can't set or change glory points on an item, except through the 'glory' command.\r\n",
              ch );
        return;
    }

    // Volk: Allows 'ownership' of items.
    if ( !str_cmp( arg2, "owner" ) ) {
        if ( xIS_SET( obj->extra_flags, ITEM_PROTOTYPE ) ) {
            send_to_char( "You can NOT set an owner on prototype objects!\r\n", ch );
            return;
        }

        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Please use oset (obj) owner (name) to set an owner. Owner cleared.",
                          ch );
            STRFREE( obj->owner );
            return;
        }

        if ( obj->owner )
            STRFREE( obj->owner );

        obj->owner = STRALLOC( arg3 );
        ch_printf( ch,
                   "New owner of this object: %s. Please note, you can NOT set an owner as a prototype.\r\n",
                   arg3 );

        return;
    }

    if ( !str_cmp( arg2, "ed" ) ) {
        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Syntax: oset <object> ed <keywords>\r\n", ch );
            return;
        }
        CHECK_SUBRESTRICTED( ch );
        if ( obj->timer ) {
            send_to_char
                ( "It's not safe to edit an extra description on an object with a timer.\r\nTurn it off first.\r\n",
                  ch );
            return;
        }
        if ( obj->item_type == ITEM_PAPER && get_trust( ch ) < LEVEL_IMMORTAL ) {
            send_to_char( "You can not add an extra description to a note paper at the moment.\r\n",
                          ch );
            return;
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            ed = SetOExtraProto( obj->pIndexData, arg3 );
        else
            ed = SetOExtra( obj, arg3 );
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        if ( lockobj )
            ch->spare_ptr = obj;
        else
            ch->spare_ptr = NULL;
        ch->substate = SUB_OBJ_EXTRA;
        ch->dest_buf = ed;
        start_editing( ch, ed->description );
        editor_desc_printf( ch, "Extra description '%s' on object vnum %d (%s).", arg3,
                            obj->pIndexData->vnum, obj->short_descr );
        return;
    }
    if ( !str_cmp( arg2, "rmed" ) ) {
        if ( !VLD_STR( arg3 ) ) {
            send_to_char( "Syntax: oset <object> rmed <keywords>\r\n", ch );
            return;
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            if ( DelOExtraProto( obj->pIndexData, arg3 ) )
                send_to_char( "Deleted.\r\n", ch );
            else
                send_to_char( "Not found.\r\n", ch );
            return;
        }
        if ( DelOExtra( obj, arg3 ) )
            send_to_char( "Deleted.\r\n", ch );
        else
            send_to_char( "Not found.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        send_to_char( "You can only oset the name, short and long right now.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;

        if ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON ) {
            send_to_char
                ( "Weapon (v1 + v2) and armor (v0 + v1) values are automatically set by the level of the object.\r\nTo override this you must get a council head to add an OVERRIDE flag.\r\n",
                  ch );
            return;
        }

        obj->value[0] = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[0] = value;
        return;
    }
    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;

        if ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_ARMOR
             || obj->item_type == ITEM_MISSILE_WEAPON ) {
            send_to_char
                ( "Weapon (v1 + v2) and armor (v0 + v1) values are automatically set by the level of the object.\r\nTo override this you must get a council head to add an OVERRIDE flag.\r\n",
                  ch );
            return;
        }

        if ( !is_number( arg3 ) && obj->item_type == ITEM_CONTAINER ) {
            value = get_containerflag( arg3 );
            TOGGLE_BIT( obj->value[1], 1 << value );
        }
        else
            obj->value[1] = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[1] = value;
        return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_ARMOR
             || obj->item_type == ITEM_MISSILE_WEAPON ) {
            send_to_char
                ( "Weapon (v1 + v2) and armor (v0 + v1) values are automatically set by the level of the object.\r\nTo override this you must get a council head to add an OVERRIDE flag.\r\n",
                  ch );
            return;
        }

        obj->value[2] = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            obj->pIndexData->value[2] = value;
/*      if(obj->item_type == ITEM_WEAPON && value != 0)
        obj->value[2] = obj->pIndexData->value[1] * obj->pIndexData->value[2];  */
        }
        return;
    }
    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->value[3] = value;
        if ( obj->item_type == ITEM_FURNITURE && value > 200 ) {
            send_to_char( "Sorry 200 is the max for furniture currently.\r\n", ch );
            obj->value[3] = 0;
            return;
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[3] = value;
        return;
    }
    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->value[4] = value;
        if ( obj->item_type == ITEM_FURNITURE && value > 200 ) {
            send_to_char( "Sorry 200 is the max for furniture currently.\r\n", ch );
            obj->value[4] = 0;
            return;
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[4] = value;
        return;
    }
    if ( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->value[5] = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[5] = value;
        return;
    }
    if ( !str_cmp( arg2, "value6" ) || !str_cmp( arg2, "v6" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->value[6] = value;

        if ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_PROJECTILE ) {
            if ( value > 100 ) {
                value = 100;                           // for the prototype check below.
                send_to_char( "Value to high, setting to 100.", ch );
            }
            obj->value[6] = value;
            // update_weapon(ch, obj);
        }

        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[6] = value;
        return;
    }
    if ( !str_cmp( arg2, "size" ) ) {

        if ( !can_omodify( ch, obj ) )
            return;

        if ( !argument || argument == '\0' ) {
            send_to_char
                ( "Usage: oset <object> size (number or tiny, small, large etc). See HELP OBJECTSIZES\r\n",
                  ch );
            return;
        }

        short                   size = -1;

        if ( is_number( argument ) ) {
            size = atoi( argument );
            if ( size < 0 || size > 7 ) {
                send_to_char( "Invalid value, 0-7 only. See HELP OBJECTSIZES.\r\n", ch );
                return;
            }
        }
        else {
            short                   x;

            for ( x = 0; x < 8; x++ )
                if ( !str_cmp( obj_sizes[x], argument ) )
                    size = x;
        }

        if ( size == -1 ) {
            send_to_char( "Invalid value. See HELP OBJECTSIZES.\r\n", ch );
            return;
        }

        obj->size = size;

        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->size = obj->size;
        send_to_char( "Done\r\n", ch );
        return;

    }
    if ( !str_cmp( arg2, "type" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: oset <object> type <type>\r\n", ch );
            return;
        }
        value = get_otype( argument );
        if ( value < 1 ) {
            ch_printf( ch, "Unknown type: %s\r\n", arg3 );
            return;
        }
        // Reset value and currtype if obj->item_type didn't already exist
        // and was tampered with.  Prevent passing a differnt type after 
        // level currtype/amt established. -Taon
        if ( obj->item_type && obj->item_type != value ) {
            send_to_char( "New type detected, setting value to 1 copper.\r\n", ch );
            send_to_char( "Reset item level, for proper adjustments.\r\n", ch );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                obj->pIndexData->cost = 1;
                obj->pIndexData->currtype = CURR_COPPER;
            }
            else {
                obj->cost = 1;
                obj->currtype = CURR_COPPER;
            }
        }

        obj->item_type = ( short ) value;

        // Keep builders from modifying v1,v2 before setting ITEM_TYPE. -Taon
        if ( ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON )
             && !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            if ( obj->value[1] != 0 || obj->value[2] != 0 ) {
                obj->value[1] = 0;
                obj->value[2] = 0;
                send_to_char
                    ( "You MUST reset objects level in order to auto reset min/max damages.\r\n",
                      ch );
            }
        }
        else if ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON ) {
            if ( obj->value[1] != 0 || obj->value[2] != 0 ) {
                obj->pIndexData->value[1] = 0;
                obj->pIndexData->value[2] = 0;
                send_to_char
                    ( "You MUST reset objects level in order to auto reset min/max damages.\r\n",
                      ch );
            }
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->item_type = obj->item_type;
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: oset <object> flags <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_oflag( arg3 );

            if ( value == ITEM_OVERRIDE && ch->level < 107 && !is_head_architect( ch ) ) {
                send_to_char( "You cannot set the override flag.\r\n", ch );
                return;
            }

            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else {
                if ( value == ITEM_PROTOTYPE && get_trust( ch ) < LEVEL_AJ_SGT
                     && !is_name( "protoflag", ch->pcdata->bestowments ) )
                    send_to_char( "You cannot change the prototype flag.\r\n", ch );
                else {
                    xTOGGLE_BIT( obj->extra_flags, value );
                    if ( value == ITEM_PROTOTYPE )
                        obj->pIndexData->extra_flags = obj->extra_flags;
                }
            }
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->extra_flags = obj->extra_flags;
        send_to_char( "Done\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "wear" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: oset <object> wear <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_wflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( obj->wear_flags, 1 << value );
        }
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->wear_flags = obj->wear_flags;
        return;
    }

/* 
   Rewrote level argument adding new changes to which handle
   damage min/maxes for weapons according to level. -Taon
 */
    // After all the changes this needs to be rewrote again. -Taon
    if ( !str_cmp( arg2, "level" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;

        obj->level = value;

        // Establish currency. -Taon
        if ( !IS_OBJ_STAT( obj, ITEM_OVERRIDE ) ) {
            obj->currtype = set_curr_type( obj, value );
            obj->cost = set_curr_amt( obj, value );
            obj->pIndexData->currtype = set_curr_type( obj, value );
            obj->pIndexData->cost = set_curr_amt( obj, value );
        }

        if ( !IS_OBJ_STAT( obj, ITEM_OVERRIDE ) ) {
            if ( obj->item_type == ITEM_ARMOR ) {
                if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                    obj->value[0] = set_min_armor( value );
                    obj->value[1] = set_max_armor( value );
                    obj->pIndexData->value[0] = set_min_armor( value );
                    obj->pIndexData->value[1] = set_max_armor( value );
                }
                else {
                    obj->value[0] = set_min_armor( value );
                    obj->value[1] = set_max_armor( value );
                    obj->pIndexData->value[0] = set_min_armor( value );
                    obj->pIndexData->value[1] = set_max_armor( value );
                }
                send_to_char
                    ( "Armor values are autoset, by according to what level you set the object to.\r\nThis may be overridden by a council head if neccessary.\r\n",
                      ch );
            }

            if ( obj->item_type == ITEM_WEAPON ) {
                if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                    obj->value[1] = set_min_chart( value );
                    obj->value[2] = set_max_chart( value );
                    obj->pIndexData->value[1] = set_min_chart( value );
                    obj->pIndexData->value[2] = set_max_chart( value );
                }
                else {
                    obj->value[1] = set_min_chart( value );
                    obj->value[2] = set_max_chart( value );
                    obj->pIndexData->value[1] = set_min_chart( value );
                    obj->pIndexData->value[2] = set_max_chart( value );
                }
                send_to_char
                    ( "Weapon damages are autoset, by according to what level you set the weapon to.\r\nThis may be overridden by a Council Head if the object is a quest weapon, or special circumstance.\r\n",
                      ch );
            }
            if ( obj->item_type == ITEM_MISSILE_WEAPON ) {
                if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                    obj->value[1] = min_archery_chart( value );
                    obj->value[2] = max_archery_chart( value );
                    obj->pIndexData->value[1] = min_archery_chart( value );
                    obj->pIndexData->value[2] = max_archery_chart( value );
                }
                else {
                    obj->value[1] = min_archery_chart( value );
                    obj->value[2] = max_archery_chart( value );
                    obj->pIndexData->value[1] = min_archery_chart( value );
                    obj->pIndexData->value[2] = max_archery_chart( value );
                }
                send_to_char
                    ( "Weapon damages are autoset, by according to what level you set the weapon to.\r\nThis may be overridden by a Council Head if the object is a quest weapon, or special ciircumstance.",
                      ch );
            }

        }
        return;
    }

    if ( !str_cmp( arg2, "weight" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->weight = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->weight = value;
        return;
    }
    if ( !str_cmp( arg2, "cost" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( ch->level < 107 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You cannot currently edit currtypes, contact the Head of\r\nArea Architects for an override flag.\r\n",
                  ch );
            return;
        }
        obj->cost = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->cost = value;
        return;
    }
    if ( !str_cmp( arg2, "currtype" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( ch->level < 107 && !is_head_architect( ch ) ) {
            send_to_char
                ( "You cannot currently edit currtypes, contact the Head of\r\nArea Architects for an override flag.\r\n",
                  ch );
            return;
        }
        obj->currtype = get_currency_type( argument );
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->currtype = get_currency_type( argument );
        return;
    }
    if ( !str_cmp( arg2, "rent" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->rent = value;
        else
            send_to_char( "Item must have prototype flag to set this value.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "layers" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->layers = value;
        else
            send_to_char( "Item must have prototype flag to set this value.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "timer" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->timer = value;
        return;
    }
    if ( !str_cmp( arg2, "actiondesc" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        if ( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) ) {
            send_to_char( "Illegal characters!\r\n", ch );
            return;
        }
        if ( VLD_STR( obj->action_desc ) )
            STRFREE( obj->action_desc );
        if ( VLD_STR( arg3 ) ) {
            obj->action_desc = STRALLOC( arg3 );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                if ( VLD_STR( obj->pIndexData->action_desc ) )
                    STRFREE( obj->pIndexData->action_desc );
                obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
            }
        }
        return;
    }
    /*
     * Crash fix and name support by Shaddai 
     */
    if ( !str_cmp( arg2, "affect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc;
        int                     bitv;

        if ( !can_omodify( ch, obj ) )
            return;
        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) || !VLD_STR( argument ) ) {
            send_to_char( "Usage: oset <object> affect <field> <value>\r\n", ch );
            return;
        }
        loc = get_atype( arg2 );
        if ( loc < 1 ) {
            ch_printf( ch, "Unknown field: %s\r\n", arg2 );
            return;
        }
        if ( loc >= APPLY_AFFECT && loc < APPLY_WEAPONSPELL ) {
            bitv = 0;
            while ( VLD_STR( argument ) ) {
                argument = one_argument( argument, arg3 );
                if ( loc == APPLY_AFFECT )
                    value = get_aflag( arg3 );
                else
                    value = get_risflag( arg3 );
                if ( value < 0 || value > 31 )
                    ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
                else
                    SET_BIT( bitv, 1 << value );
            }
            if ( !bitv )
                return;
            value = bitv;
        }
        else {
            one_argument( argument, arg3 );
            if ( loc == APPLY_WEARSPELL && !is_number( arg3 ) ) {
                value = skill_lookup( arg3 );
                if ( value == -1 ) {
/*          printf("%s\r\n", arg3);  */
                    send_to_char( "Unknown spell name.\r\n", ch );
                    return;
                }
            }
            else
                value = atoi( arg3 );
        }
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = loc;
        paf->modifier = value;
        xCLEAR_BITS( paf->bitvector );
        paf->next = NULL;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            LINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
        else
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        ++top_affect;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "rmaffect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc,
                                count;

        if ( !can_omodify( ch, obj ) )
            return;
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: oset <object> rmaffect <affect#>\r\n", ch );
            return;
        }
        loc = atoi( argument );
        if ( loc < 1 ) {
            send_to_char( "Invalid number.\r\n", ch );
            return;
        }
        count = 0;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
            OBJ_INDEX_DATA         *pObjIndex;

            pObjIndex = obj->pIndexData;
            for ( paf = pObjIndex->first_affect; paf; paf = paf->next ) {
                if ( ++count == loc ) {
                    UNLINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
                    DISPOSE( paf );
                    send_to_char( "Removed.\r\n", ch );
                    --top_affect;
                    return;
                }
            }
            send_to_char( "Not found.\r\n", ch );
            return;
        }
        else {
            for ( paf = obj->first_affect; paf; paf = paf->next ) {
                if ( ++count == loc ) {
                    UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
                    DISPOSE( paf );
                    send_to_char( "Removed.\r\n", ch );
                    --top_affect;
                    return;
                }
            }
            send_to_char( "Not found.\r\n", ch );
            return;
        }
    }
    /*
     * save some finger-leather 
     */
    if ( !str_cmp( arg2, "ris" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_oset( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_oset( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "r" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "i" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "s" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "ri" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_oset( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "rs" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_oset( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "is" ) ) {
        if ( !can_omodify( ch, obj ) )
            return;
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_oset( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_oset( ch, outbuf );
        return;
    }
    /*
     * Make it easier to set special object values by name than number
     * *       -Thoric
     */
    tmp = -1;
    switch ( obj->item_type ) {
        case ITEM_PROJECTILE:
            if ( !str_cmp( arg2, "missiletype" ) ) {
                unsigned int            x;

                value = -1;
                for ( x = 0; x < sizeof( projectiles ) / sizeof( projectiles[0] ); x++ )
                    if ( !str_cmp( arg3, projectiles[x] ) )
                        value = x;
                if ( value < 0 ) {
                    send_to_char( "Unknown projectile type.\r\n", ch );
                    return;
                }
                tmp = 4;
                break;
            }
            if ( !str_cmp( arg2, "damtype" ) ) {
                unsigned int            x;

                value = -1;
                for ( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
                    if ( !str_cmp( arg3, attack_table[x] ) )
                        value = x;
                if ( value < 0 ) {
                    send_to_char( "Unknown damage type.\r\n", ch );
                    return;
                }
                tmp = 3;
                break;
            }
        case ITEM_WEAPON:
            if ( !str_cmp( arg2, "weapontype" ) ) {
                unsigned int            x;

                value = -1;
                for ( x = 0; x < sizeof( weapon_skills ) / sizeof( weapon_skills[0] ); x++ )
                    if ( !str_cmp( arg3, weapon_skills[x] ) )
                        value = x;
                if ( value < 0 ) {
                    send_to_char( "Unknown weapon type.\r\n", ch );
                    return;
                }
                tmp = 4;
                break;
            }
            if ( !str_cmp( arg2, "damtype" ) ) {
                unsigned int            x;

                value = -1;
                for ( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
                    if ( !str_cmp( arg3, attack_table[x] ) )
                        value = x;
                if ( value < 0 ) {
                    send_to_char( "Unknown damage type.\r\n", ch );
                    return;
                }
                tmp = 3;
                break;
            }
            if ( !str_cmp( arg2, "condition" ) )
                tmp = 0;
            break;
        case ITEM_ARMOR:
            if ( !str_cmp( arg2, "condition" ) )
                tmp = 3;
            if ( !str_cmp( arg2, "ac" ) )
                tmp = 1;
            break;
        case ITEM_SALVE:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "maxdoses" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "doses" ) )
                tmp = 2;
            if ( !str_cmp( arg2, "delay" ) )
                tmp = 3;
            if ( !str_cmp( arg2, "spell1" ) )
                tmp = 4;
            if ( !str_cmp( arg2, "spell2" ) )
                tmp = 6;
            if ( tmp >= 4 && tmp <= 6 )
                value = skill_lookup( arg3 );
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "spell1" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "spell2" ) )
                tmp = 2;
            if ( !str_cmp( arg2, "spell3" ) )
                tmp = 3;
            if ( tmp >= 1 && tmp <= 3 )
                value = skill_lookup( arg3 );
            break;
        case ITEM_STAFF:
        case ITEM_WAND:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "spell" ) ) {
                tmp = 3;
                value = skill_lookup( arg3 );
            }
            if ( !str_cmp( arg2, "maxcharges" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "charges" ) )
                tmp = 2;
            break;
        case ITEM_CONTAINER:
            if ( !str_cmp( arg2, "capacity" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "cflags" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "key" ) )
                tmp = 2;
            break;
        case ITEM_SWITCH:
        case ITEM_LEVER:
        case ITEM_PULLCHAIN:
        case ITEM_BUTTON:
            if ( !str_cmp( arg2, "tflags" ) ) {
                tmp = 0;
                value = get_trigflag( arg3 );
            }
            break;
    }
    if ( tmp >= 0 && tmp <= 3 ) {
        if ( !can_omodify( ch, obj ) )
            return;
        obj->value[tmp] = value;
        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            obj->pIndexData->value[tmp] = value;
        return;
    }
    if ( ch->substate == SUB_REPEATCMD ) {
        ch->substate = SUB_RESTRICTED;
        interpret( ch, origarg );
        ch->substate = SUB_REPEATCMD;
        ch->last_cmd = do_oset;
    }
    else
        do_oset( ch, ( char * ) "" );
    return;
}

/* Obsolete Merc room editing routine */
void do_rset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL];
    ROOM_INDEX_DATA        *location;
    int                     value;
    bool                    proto;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    mudstrlcpy( arg3, argument, MIL );
    if ( !VLD_STR( arg1 ) || !VLD_STR( arg2 ) || !VLD_STR( arg3 ) ) {
        send_to_char( "Syntax: rset <location> <field> value\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  flags sector\r\n", ch );
        return;
    }

    if ( ( location = find_location( ch, arg1 ) ) == NULL ) {
        send_to_char( "No such location.\r\n", ch );
        return;
    }

    if ( !can_rmodify( ch, location ) )
        return;

    if ( !is_number( arg3 ) ) {
        send_to_char( "Value must be numeric.\r\n", ch );
        return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "flags" ) ) {
        /*
         * Protect from messing up prototype flag
         */
        if ( IS_SET( location->room_flags, ROOM_PROTOTYPE ) )
            proto = TRUE;
        else
            proto = FALSE;
        location->room_flags = value;
        if ( proto )
            SET_BIT( location->room_flags, ROOM_PROTOTYPE );
        return;
    }

    if ( !str_cmp( arg2, "sector" ) ) {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_rset( ch, ( char * ) "" );
    return;
}

/* Returns value 0 - 9 based on directional text. */
int get_dir( char *txt )
{
    int                     edir;
    char                    c1,
                            c2;

    if ( !str_cmp( txt, "northeast" ) )
        return DIR_NORTHEAST;
    if ( !str_cmp( txt, "northwest" ) )
        return DIR_NORTHWEST;
    if ( !str_cmp( txt, "southeast" ) )
        return DIR_SOUTHEAST;
    if ( !str_cmp( txt, "southwest" ) )
        return DIR_SOUTHWEST;
    if ( !str_cmp( txt, "somewhere" ) )
        return DIR_SOMEWHERE;
    if ( ( ( txt[0] == 'e' || txt[0] == 'E' ) && txt[1] == 'x' ) || !str_cmp( txt, "11" ) )
        return DIR_EXPLORE;
    c1 = txt[0];
    if ( c1 == '\0' )
        return 0;
    c2 = txt[1];
    edir = 0;
    switch ( c1 ) {
        case 'n':
            switch ( c2 ) {
                default:
                    edir = 0;
                    break;                             /* north */
                case 'e':
                    edir = 6;
                    break;                             /* ne */
                case 'w':
                    edir = 7;
                    break;                             /* nw */
            }
            break;
        case '0':
            edir = 0;
            break;                                     /* north */

        case 'e':
        case '1':
            edir = 1;
            break;                                     /* east */

        case 's':
            switch ( c2 ) {
                default:
                    edir = 2;
                    break;                             /* south */
                case 'e':
                    edir = 8;
                    break;                             /* se */
                case 'w':
                    edir = 9;
                    break;                             /* sw */
            }
            break;
        case '2':
            edir = 2;
            break;                                     /* south */
        case 'w':
        case '3':
            edir = 3;
            break;                                     /* west */
        case 'u':
        case '4':
            edir = 4;
            break;                                     /* up */
        case 'd':
        case '5':
            edir = 5;
            break;                                     /* down */
        case '6':
            edir = 6;
            break;                                     /* ne */
        case '7':
            edir = 7;
            break;                                     /* nw */
        case '8':
            edir = 8;
            break;                                     /* se */
        case '9':
            edir = 9;
            break;                                     /* sw */
        case '?':
            edir = 10;
            break;                                     /* somewhere */
    }
    return edir;
}

void do_redit( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            buf[MSL];

//  char *arg4[MIL];
    ROOM_INDEX_DATA        *location,
                           *tmp;
    EXTRA_DESCR_DATA       *ed;
    char                    dir = '\0';
    EXIT_DATA              *xit,
                           *texit;
    int                     value,
                            edir = 0,
        ekey,
        evnum;
    char                   *origarg = argument;

    set_char_color( AT_PLAIN, ch );

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor.\r\n", ch );
        return;
    }
    switch ( ch->substate ) {
        default:
            break;
        case SUB_ROOM_DESC:
            location = ( ROOM_INDEX_DATA * ) ch->dest_buf;
            if ( !location ) {
                bug( "redit: sub_room_desc: NULL ch->dest_buf" );
                location = ch->in_room;
            }
            if ( VLD_STR( location->description ) )
                STRFREE( location->description );
            location->description = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;

            if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                send_to_char( "!!SOUND(sound/queen.wav)\r\n", ch );

            return;
        case SUB_ROOM_EXTRA:
            ed = ( EXTRA_DESCR_DATA * ) ch->dest_buf;
            if ( !ed ) {
                bug( "redit: sub_room_extra: NULL ch->dest_buf" );
                stop_editing( ch );
                return;
            }
            if ( VLD_STR( ed->description ) )
                STRFREE( ed->description );
            ed->description = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            return;
    }
    location = ch->in_room;
    smash_tilde( argument );
    argument = one_argument( argument, arg );
    if ( ch->substate == SUB_REPEATCMD ) {
        if ( !VLD_STR( arg ) ) {
            do_rstat( ch, ( char * ) "" );
            return;
        }
        if ( !str_cmp( arg, "done" ) || !str_cmp( arg, "off" ) ) {
            send_to_char( "Redit mode off.\r\n", ch );
            if ( ch->pcdata && VLD_STR( ch->pcdata->subprompt ) )
                STRFREE( ch->pcdata->subprompt );
            ch->substate = SUB_NONE;
            return;
        }
    }
    if ( !VLD_STR( arg ) || !str_cmp( arg, "?" ) ) {
        if ( ch->substate == SUB_REPEATCMD )
            send_to_char( "Syntax: <field> value\r\n", ch );
        else
            send_to_char( "Syntax: redit <field> value\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  name desc ed rmed currvnum\r\n", ch );
        send_to_char( "  exit bexit exdesc exflags exname exkey\r\n", ch );
        send_to_char( "  flags sector teledelay televnum tunnel\r\n", ch );
        send_to_char( "  rlist pulltype pull push height\r\n", ch );
//    send_to_char("  RESOURCES: restore maxresources rarity\r\n", ch);
        return;
    }
    if ( !can_rmodify( ch, location ) )
        return;
    if ( !str_cmp( arg, "on" ) ) {
        send_to_char( "Redit mode on.\r\n", ch );
        ch->substate = SUB_REPEATCMD;
        if ( ch->pcdata ) {
            if ( VLD_STR( ch->pcdata->subprompt ) )
                STRFREE( ch->pcdata->subprompt );
            ch->pcdata->subprompt = STRALLOC( "<&CRedit &W#%r&w> %i" );
        }
        return;
    }

    if ( ch->in_room->sector_type == SECT_NOCHANGE && ch->level < LEVEL_AJ_COLONEL ) {
        send_to_char( "\r\nThis room may not be changed without Vladaar's permission.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "name" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "Set the room name.  A very brief single line room description.\r\n",
                          ch );
            send_to_char( "Usage: redit name <Room summary>\r\n", ch );
            return;
        }
        if ( VLD_STR( location->name ) )
            STRFREE( location->name );
        location->name = STRALLOC( argument );
        return;
    }
    if ( !str_cmp( arg, "desc" ) ) {
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_ROOM_DESC;
        ch->dest_buf = location;
        start_editing( ch, location->description );
        editor_desc_printf( ch, "Description of room vnum %d (%s).", location->vnum,
                            location->name );
        return;
    }
    if ( !str_cmp( arg, "tunnel" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char
                ( "Set the maximum characters allowed in the room at one time. (0 = unlimited).\r\n",
                  ch );
            send_to_char( "Usage: redit tunnel <value>\r\n", ch );
            return;
        }
        location->tunnel = URANGE( 0, atoi( argument ), 1000 );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "height" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Set the minimum height needed to enter a room. (0 = no limit).\r\n",
                          ch );
            send_to_char( "Usage: redit height <value>\r\n", ch );
            return;
        }
        location->height = URANGE( 0, atoi( argument ), 10000 );
        send_to_char( "Done.\r\n", ch );
        return;
    }
/* Trade support! 
  if(!str_cmp(arg, "maxresources")) 
  {
    if(!argument || argument[0] == '\0')
    {
      send_to_char("Maximum Resources in the room (based on sector). Max 20.\r\n", ch);
      return;
    }
    location->maxresources = URANGE(0, atoi(argument), 20);
    send_to_char("Done.\r\n", ch);
    return;
  }

  if(!str_cmp(arg, "restore"))
  {
    location->curresources = location->maxresources;
    send_to_char("Current resources have been set to maximum!\r\n", ch);
    return;
  }

  if(!str_cmp(arg, "rarity"))
  {
    if(!argument || argument[0] == '\0')
    {
      send_to_char("Rarity of resources in room (1 - 10).\r\n", ch);
      return;
    }
    location->rarity = URANGE(0, atoi(argument), 10);
    return;
  }
*/
    /*
     * Crash fix and name support by Shaddai 
     */
    if ( !str_cmp( arg, "affect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc;
        int                     bitv;

        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) || !VLD_STR( argument ) ) {
            send_to_char( "Usage: redit affect <field> <value>\r\n", ch );
            return;
        }
        loc = get_atype( arg2 );
        if ( loc < 1 ) {
            ch_printf( ch, "Unknown field: %s\r\n", arg2 );
            return;
        }
        if ( loc >= APPLY_AFFECT && loc < APPLY_WEAPONSPELL ) {
            bitv = 0;
            while ( argument[0] != '\0' ) {
                argument = one_argument( argument, arg3 );
                if ( loc == APPLY_AFFECT )
                    value = get_aflag( arg3 );
                else
                    value = get_risflag( arg3 );
                if ( value < 0 || value > 31 )
                    ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
                else
                    SET_BIT( bitv, 1 << value );
            }
            if ( !bitv )
                return;
            value = bitv;
        }
        else {
            one_argument( argument, arg3 );
            if ( loc == APPLY_WEARSPELL && !is_number( arg3 ) ) {
                value = skill_lookup( arg3 );
//value = bsearch_skill_exact(arg3, gsn_first_spell, gsn_first_skill - 1);
                if ( value == -1 ) {
                    send_to_char( "Unknown spell name.\r\n", ch );
                    return;
                }
            }
            else
                value = atoi( arg3 );
        }
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = loc;
        paf->modifier = value;
        xCLEAR_BITS( paf->bitvector );
        paf->next = NULL;
        LINK( paf, location->first_affect, location->last_affect, next, prev );
        ++top_affect;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "rmaffect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc,
                                count;

        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: redit rmaffect <affect#>\r\n", ch );
            return;
        }
        loc = atoi( argument );
        if ( loc < 1 ) {
            send_to_char( "Invalid number.\r\n", ch );
            return;
        }
        count = 0;
        for ( paf = location->first_affect; paf; paf = paf->next ) {
            if ( ++count == loc ) {
                UNLINK( paf, location->first_affect, location->last_affect, next, prev );
                DISPOSE( paf );
                send_to_char( "Removed.\r\n", ch );
                --top_affect;
                return;
            }
        }
        send_to_char( "Not found.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "ed" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Create an extra description.\r\n", ch );
            send_to_char( "You must supply keyword(s).\r\n", ch );
            return;
        }
        CHECK_SUBRESTRICTED( ch );
        ed = SetRExtra( location, argument );
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_ROOM_EXTRA;
        ch->dest_buf = ed;
        start_editing( ch, ed->description );
        editor_desc_printf( ch, "Extra description '%s' on room %d (%s).", argument, location->vnum,
                            location->name );
        return;
    }
    if ( !str_cmp( arg, "rmed" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Remove an extra description.\r\n", ch );
            send_to_char( "You must supply keyword(s).\r\n", ch );
            return;
        }
        if ( DelRExtra( location, argument ) )
            send_to_char( "Deleted.\r\n", ch );
        else
            send_to_char( "Not found.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "rlist" ) ) {
        RESET_DATA             *pReset;
        char                   *rbuf;
        short                   num;

        if ( !location->first_reset ) {
            send_to_char( "This room has no resets to list.\r\n", ch );
            return;
        }
        num = 0;
        for ( pReset = location->first_reset; pReset; pReset = pReset->next ) {
            ++num;
            if ( !( rbuf = sprint_reset( pReset, &num ) ) )
                continue;
            send_to_char( rbuf, ch );
        }
        return;
    }
    if ( !str_cmp( arg, "flags" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Toggle the room flags.\r\n", ch );
            send_to_char( "Usage: redit flags <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg2 );
            value = get_rflag( arg2 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
            else {
                if ( 1 << value == ROOM_CLANSTOREROOM )
                    if ( IS_SET( location->room_flags, ROOM_CLANSTOREROOM ) ) {
//            char buf[MSL];
                        FILE                   *fp;

                        snprintf( buf, MSL, "%s%d.vault", STORAGE_DIR, location->vnum );
                        if ( ( fp = FileOpen( buf, "w" ) ) != NULL )
                            FileClose( fp );
                    }
                if ( 1 << value == ROOM_PROTOTYPE && get_trust( ch ) < LEVEL_IMMORTAL ) {
                    send_to_char( "You cannot change the prototype flag.\r\n", ch );
                    return;
                }
                else
                    TOGGLE_BIT( location->room_flags, 1 << value );
            }
        }
        return;
    }
    if ( !str_cmp( arg, "teledelay" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Set the delay of the teleport. (0 = off).\r\n", ch );
            send_to_char( "Usage: redit teledelay <value>\r\n", ch );
            return;
        }
        location->tele_delay = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "televnum" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Set the vnum of the room to teleport to.\r\n", ch );
            send_to_char( "Usage: redit televnum <vnum>\r\n", ch );
            return;
        }
        location->tele_vnum = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "sector" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Set the sector type.\r\n", ch );
            send_to_char( "Usage: redit sector <value>\r\n", ch );
            return;
        }
        location->sector_type = atoi( argument );
        if ( location->sector_type < 0 || location->sector_type >= SECT_MAX ) {
            location->sector_type = 1;
            send_to_char( "Out of range.\r\n", ch );
        }
        else
            send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "exkey" ) ) {
        argument = one_argument( argument, arg2 );
        argument = one_argument( argument, arg3 );
        if ( !VLD_STR( arg2 ) || !VLD_STR( arg3 ) ) {
            send_to_char( "Usage: redit exkey <dir> <key vnum>\r\n", ch );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        value = atoi( arg3 );
        if ( !xit ) {
            send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
            return;
        }
        xit->key = value;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "exname" ) ) {
        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Change or clear exit keywords.\r\n", ch );
            send_to_char( "Usage: redit exname <dir> [keywords]\r\n", ch );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( !xit ) {
            send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
            return;
        }
        if ( VLD_STR( xit->keyword ) )
            STRFREE( xit->keyword );
        xit->keyword = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "exflags" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Toggle or display exit flags.\r\n", ch );
            send_to_char( "Usage: redit exflags <dir> <flag> [flag]...\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg2 );
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( !xit ) {
            send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
            return;
        }
        if ( !VLD_STR( argument ) ) {
            snprintf( buf, MSL, "Flags for exit direction: %d  Keywords: %s  Key: %d\r\n[ ",
                      xit->vdir, xit->keyword, xit->key );
            for ( value = 0; value <= MAX_EXFLAG; value++ ) {
                if ( IS_SET( xit->exit_info, 1 << value ) ) {
                    mudstrlcat( buf, ex_flags[value], MSL );
                    mudstrlcat( buf, " ", MSL );
                }
            }
            mudstrlcat( buf, "]\r\n", MSL );
            send_to_char( buf, ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg2 );
            value = get_exflag( arg2 );
            if ( value < 0 || value > MAX_EXFLAG )
                ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
            else
                TOGGLE_BIT( xit->exit_info, 1 << value );
        }
        return;
    }
    if ( !str_cmp( arg, "ex_flags" ) ) {
        argument = one_argument( argument, arg2 );

        value = get_exflag( arg2 );
        if ( value < 0 ) {
            send_to_char( "Bad exit flag. \r\n", ch );
            return;
        }
        if ( ( xit = get_exit( location, edir ) ) == NULL ) {
            snprintf( buf, MSL, "exit %c 1", dir );
            do_redit( ch, buf );
            xit = get_exit( location, edir );
        }
        TOGGLE_BIT( xit->exit_info, 1 << value );
        return;
    }
    if ( !str_cmp( arg, "ex_to_room" ) ) {
        argument = one_argument( argument, arg2 );
        evnum = atoi( arg2 );
        if ( evnum < 1 || evnum > MAX_VNUM ) {
            send_to_char( "Invalid room number.\r\n", ch );
            return;
        }
        if ( ( tmp = get_room_index( evnum ) ) == NULL ) {
            send_to_char( "Non-existant room.\r\n", ch );
            return;
        }
        if ( ( xit = get_exit( location, edir ) ) == NULL ) {
            snprintf( buf, MSL, "exit %c 1", dir );
            do_redit( ch, buf );
            xit = get_exit( location, edir );
        }
        xit->vnum = evnum;
        return;
    }
    if ( !str_cmp( arg, "ex_key" ) ) {
        argument = one_argument( argument, arg2 );
        if ( ( xit = get_exit( location, edir ) ) == NULL ) {
            snprintf( buf, MSL, "exit %c 1", dir );
            do_redit( ch, buf );
            xit = get_exit( location, edir );
        }
        xit->key = atoi( arg2 );
        return;
    }
    if ( !str_cmp( arg, "ex_exdesc" ) ) {
        if ( ( xit = get_exit( location, edir ) ) == NULL ) {
            snprintf( buf, MSL, "exit %c 1", dir );
            do_redit( ch, buf );
        }
        snprintf( buf, MSL, "exdesc %c %s", dir, argument );
        do_redit( ch, buf );
        return;
    }
    if ( !str_cmp( arg, "ex_keywords" ) ) {            /* not called yet */
        if ( ( xit = get_exit( location, edir ) ) == NULL ) {
            snprintf( buf, MSL, "exit %c 1", dir );
            do_redit( ch, buf );
            if ( ( xit = get_exit( location, edir ) ) == NULL )
                return;
        }
        if ( VLD_STR( xit->keyword ) ) {
            snprintf( buf, MSL, "%s %s", xit->keyword, argument );
            STRFREE( xit->keyword );
        }
        else
            snprintf( buf, MSL, "%s", argument );
        xit->keyword = STRALLOC( buf );
        return;
    }
    if ( !str_cmp( arg, "exit" ) ) {
        bool                    addexit,
                                numnotdir;

        argument = one_argument( argument, arg2 );
        argument = one_argument( argument, arg3 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Create, change or remove an exit.\r\n", ch );
            send_to_char( "Usage: redit exit <dir> [room] [flags] [key] [keywords]\r\n", ch );
            return;
        }
        addexit = numnotdir = FALSE;
        switch ( arg2[0] ) {
            default:
                edir = get_dir( arg2 );
                break;
            case '+':
                edir = get_dir( arg2 + 1 );
                addexit = TRUE;
                break;
            case '#':
                edir = atoi( arg2 + 1 );
                numnotdir = TRUE;
                break;
        }
        if ( !arg3 || arg3[0] == '\0' )
            evnum = 0;
        else
            evnum = atoi( arg3 );
        if ( numnotdir ) {
            if ( ( xit = get_exit_num( location, edir ) ) != NULL )
                edir = xit->vdir;
        }
        else
            xit = get_exit( location, edir );
        if ( !evnum ) {
            if ( xit ) {
                extract_exit( location, xit );
                send_to_char( "Exit removed.\r\n", ch );
                return;
            }
            send_to_char( "No exit in that direction.\r\n", ch );
            return;
        }
        if ( evnum < 1 || evnum > MAX_VNUM ) {
            send_to_char( "Invalid room number.\r\n", ch );
            return;
        }
        if ( ( tmp = get_room_index( evnum ) ) == NULL ) {
            send_to_char( "Non-existant room.\r\n", ch );
            return;
        }
        if ( addexit || !xit ) {
            if ( numnotdir ) {
                send_to_char( "Cannot add an exit by number, sorry.\r\n", ch );
                return;
            }
            if ( addexit && xit && get_exit_to( location, edir, tmp->vnum ) ) {
                send_to_char
                    ( "There is already an exit in that direction leading to that location.\r\n",
                      ch );
                return;
            }
            xit = make_exit( location, tmp, edir );
            xit->key = -1;
            xit->exit_info = 0;
            if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) && !IS_NPC( ch ) ) {
                act( AT_IMMORT, "$n reveals a hidden passage!", ch, NULL, NULL, TO_ROOM );
            }
        }
        else
            act( AT_IMMORT, "Something is different...", ch, NULL, NULL, TO_ROOM );
        if ( xit->to_room != tmp ) {
            xit->to_room = tmp;
            xit->vnum = evnum;
            texit = get_exit_to( xit->to_room, rev_dir[edir], location->vnum );
            if ( texit ) {
                texit->rexit = xit;
                xit->rexit = texit;
            }
        }
        argument = one_argument( argument, arg3 );
        if ( arg3 && arg3[0] != '\0' )
            xit->exit_info = atoi( arg3 );
        if ( VLD_STR( argument ) ) {
            one_argument( argument, arg3 );
            ekey = atoi( arg3 );
            if ( ekey != 0 || arg3[0] == '0' ) {
                argument = one_argument( argument, arg3 );
                xit->key = ekey;
            }
            if ( VLD_STR( argument ) ) {
                if ( VLD_STR( xit->keyword ) )
                    STRFREE( xit->keyword );
                xit->keyword = STRALLOC( argument );
            }
        }
        return;
    }
    if ( !str_cmp( arg, "bexit" ) ) {
        EXIT_DATA              *rxit;
        char                    tmpcmd[MIL];
        ROOM_INDEX_DATA        *tmploc;
        int                     vnum,
                                exnum;
        char                    rvnum[MIL];
        bool                    numnotdir;

        argument = one_argument( argument, arg2 );
        argument = one_argument( argument, arg3 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Create, change or remove a two-way exit.\r\n", ch );
            send_to_char( "Usage: redit bexit <dir> [room] [flags] [key] [keywords]\r\n", ch );
            return;
        }
        numnotdir = FALSE;
        switch ( arg2[0] ) {
            default:
                edir = get_dir( arg2 );
                break;

            case '#':
                numnotdir = TRUE;
                edir = atoi( arg2 + 1 );
                break;

            case '+':
                edir = get_dir( arg2 + 1 );
                break;
        }
        tmploc = location;
        exnum = edir;
        if ( numnotdir ) {
            if ( ( xit = get_exit_num( tmploc, edir ) ) != NULL )
                edir = xit->vdir;
        }
        else
            xit = get_exit( tmploc, edir );
        rxit = NULL;
        vnum = 0;
        rvnum[0] = '\0';
        if ( xit ) {
            vnum = xit->vnum;
            if ( VLD_STR( arg3 ) )
                snprintf( rvnum, MIL, "%d", tmploc->vnum );
            if ( xit->to_room )
                rxit = get_exit( xit->to_room, rev_dir[edir] );
            else
                rxit = NULL;
        }
        snprintf( tmpcmd, MIL, "exit %s %s %s", arg2, arg3, argument );
        do_redit( ch, tmpcmd );
        if ( numnotdir )
            xit = get_exit_num( tmploc, exnum );
        else
            xit = get_exit( tmploc, edir );
        if ( !rxit && xit ) {
            vnum = xit->vnum;
            if ( VLD_STR( arg3 ) )
                snprintf( rvnum, MIL, "%d", tmploc->vnum );
            if ( xit->to_room )
                rxit = get_exit( xit->to_room, rev_dir[edir] );
            else
                rxit = NULL;
        }
        if ( vnum ) {
            snprintf( tmpcmd, MIL, "%d redit exit %d %s %s", vnum, rev_dir[edir], rvnum, argument );
            do_at( ch, tmpcmd );
        }
        return;
    }
    if ( !str_cmp( arg, "pulltype" ) || !str_cmp( arg, "pushtype" ) ) {
        int                     pt;

        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) ) {
            ch_printf( ch, "Set the %s between this room, and the destination room.\r\n", arg );
            ch_printf( ch, "Usage: redit %s <dir> <type>\r\n", arg );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( xit ) {
            if ( ( pt = get_pulltype( argument ) ) == -1 )
                ch_printf( ch, "Unknown pulltype: %s.  (See help PULLTYPES)\r\n", argument );
            else {
                xit->pulltype = pt;
                return;
            }
        }
        send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "pull" ) ) {
        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Set the 'pull' between this room, and the destination room.\r\n", ch );
            send_to_char( "Usage: redit pull <dir> <force (0 to 100)>\r\n", ch );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( xit ) {
            xit->pull = URANGE( -100, atoi( argument ), 100 );
            send_to_char( "Done.\r\n", ch );
            return;
        }
        send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "push" ) ) {
        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char
                ( "Set the 'push' away from the destination room in the opposite direction.\r\n",
                  ch );
            send_to_char( "Usage: redit push <dir> <force (0 to 100)>\r\n", ch );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( xit ) {
            xit->pull = URANGE( -100, -( atoi( argument ) ), 100 );
            send_to_char( "Done.\r\n", ch );
            return;
        }
        send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "exdesc" ) ) {
        argument = one_argument( argument, arg2 );
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Create or clear a description for an exit.\r\n", ch );
            send_to_char( "Usage: redit exdesc <dir> [description]\r\n", ch );
            return;
        }
        if ( arg2[0] == '#' ) {
            edir = atoi( arg2 + 1 );
            xit = get_exit_num( location, edir );
        }
        else {
            edir = get_dir( arg2 );
            xit = get_exit( location, edir );
        }
        if ( xit ) {
            if ( VLD_STR( xit->description ) )
                STRFREE( xit->description );
            if ( VLD_STR( argument ) ) {
                snprintf( buf, MSL, "%s\r\n", argument );
                xit->description = STRALLOC( buf );
            }
            send_to_char( "Done.\r\n", ch );
            return;
        }
        send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "currvnum" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Set the currency vnum.\r\n", ch );
            send_to_char( "Usage: rset currvnum <vnum>\r\n", ch );
            return;
        }
        location->currvnum = atoi( argument );
        assign_currindex( location );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( ch->substate == SUB_REPEATCMD ) {
        ch->substate = SUB_RESTRICTED;
        interpret( ch, origarg );
        ch->substate = SUB_REPEATCMD;
        ch->last_cmd = do_redit;
    }
    else
        do_redit( ch, ( char * ) "" );
    return;
}

void do_ocreate( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;
    int                     vnum,
                            cvnum;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobiles cannot create.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    vnum = is_number( arg ) ? atoi( arg ) : -1;

    if ( vnum == -1 || !argument || argument[0] == '\0' ) {
        send_to_char( "Usage:  ocreate <vnum> [copy vnum] <item name>\r\n", ch );
        return;
    }

    if ( vnum < 1 || vnum > MAX_VNUM ) {
        send_to_char( "Vnum out of range.\r\n", ch );
        return;
    }

    one_argument( argument, arg2 );
    cvnum = atoi( arg2 );
    if ( cvnum != 0 )
        argument = one_argument( argument, arg2 );
    if ( cvnum < 1 )
        cvnum = 0;

    if ( get_obj_index( vnum ) ) {
        send_to_char( "An object with that number already exists.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        return;
    if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        AREA_DATA              *pArea;

        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to create objects.\r\n", ch );
            return;
        }
        if ( vnum < pArea->low_o_vnum || vnum > pArea->hi_o_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }

    pObjIndex = make_object( vnum, cvnum, argument );
    pObjIndex->area = ch->pcdata->area;

    if ( !pObjIndex ) {
        send_to_char( "Error.\r\n", ch );
        log_string( "do_ocreate: make_object failed." );
        return;
    }
    obj = create_object( pObjIndex, get_trust( ch ) );
    obj_to_char( obj, ch );
    act( AT_IMMORT, "$n makes arcane gestures, and opens $s hands to reveal $p!", ch, obj, NULL,
         TO_ROOM );
    ch_printf( ch,
               "&YYou make arcane gestures, and open your hands to reveal %s!\r\nObjVnum:  &W%d   &YKeywords:  &W%s\r\n",
               pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name );
}

void do_mcreate( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    MOB_INDEX_DATA         *pMobIndex;
    CHAR_DATA              *mob;
    int                     vnum,
                            cvnum;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobiles cannot create.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    vnum = is_number( arg ) ? atoi( arg ) : -1;

    if ( vnum == -1 || !argument || argument[0] == '\0' ) {
        send_to_char( "Usage:  mcreate <vnum> [cvnum] <mobile name>\r\n", ch );
        return;
    }

    if ( vnum < 1 || vnum > MAX_VNUM ) {
        send_to_char( "Vnum out of range.\r\n", ch );
        return;
    }

    one_argument( argument, arg2 );
    cvnum = atoi( arg2 );
    if ( cvnum != 0 )
        argument = one_argument( argument, arg2 );
    if ( cvnum < 1 )
        cvnum = 0;

    if ( get_mob_index( vnum ) ) {
        send_to_char( "A mobile with that number already exists.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        return;
    if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        AREA_DATA              *pArea;

        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to create mobiles.\r\n", ch );
            return;
        }
        if ( vnum < pArea->low_m_vnum || vnum > pArea->hi_m_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }

    pMobIndex = make_mobile( vnum, cvnum, argument );
    pMobIndex->area = ch->pcdata->area;

    if ( !pMobIndex ) {
        send_to_char( "Error.\r\n", ch );
        log_string( "do_mcreate: make_mobile failed." );
        return;
    }
    mob = create_mobile( pMobIndex );
    char_to_room( mob, ch->in_room );
    act( AT_IMMORT, "$n waves $s arms about, and $N appears at $s command!", ch, NULL, mob,
         TO_ROOM );
    ch_printf( ch,
               "&YYou wave your arms about, and %s appears at your command!\r\nMobVnum:  &W%d   &YKeywords:  &W%s\r\n",
               pMobIndex->short_descr, pMobIndex->vnum, pMobIndex->player_name );
}

void assign_area( CHAR_DATA *ch, bool quiet )
{
    char                    buf[MSL];
    char                    buf2[MSL];
    char                    taf[1024];
    AREA_DATA              *tarea,
                           *tmp;
    bool                    created = FALSE;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Npc's have no use for an area.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && ch->pcdata->r_range_lo && ch->pcdata->r_range_hi ) {
        tarea = ch->pcdata->area;
        snprintf( taf, 1024, "%s.are", capitalize( ch->name ) );
        if ( !tarea ) {
            for ( tmp = first_build; tmp; tmp = tmp->next )
                if ( !str_cmp( taf, tmp->filename ) ) {
                    tarea = tmp;
                    break;
                }
        }
        if ( !tarea ) {
            snprintf( buf, MSL, "Creating area entry for %s", ch->name );
            if ( !quiet )
                log_string_plus( buf, LOG_NORMAL, ch->level );
            CREATE( tarea, AREA_DATA, 1 );

            LINK( tarea, first_build, last_build, next, prev );
            tarea->first_room = tarea->last_room = NULL;
            snprintf( buf, MSL, "{PROTO} %s's area in progress", ch->name );
            tarea->name = STRALLOC( buf );
            tarea->filename = STRALLOC( taf );
            snprintf( buf2, MSL, "%s", ch->name );
            tarea->author = STRALLOC( buf2 );
            tarea->age = 0;
            tarea->nplayer = 0;
            created = TRUE;
        }
        else {
            snprintf( buf, MSL, "Updating area entry for %s", ch->name );
            if ( !quiet )
                log_string_plus( buf, LOG_NORMAL, ch->level );
        }
        tarea->low_r_vnum = ch->pcdata->r_range_lo;
        tarea->low_o_vnum = ch->pcdata->o_range_lo;
        tarea->low_m_vnum = ch->pcdata->m_range_lo;
        tarea->hi_r_vnum = ch->pcdata->r_range_hi;
        tarea->hi_o_vnum = ch->pcdata->o_range_hi;
        tarea->hi_m_vnum = ch->pcdata->m_range_hi;
        ch->pcdata->area = tarea;
        if ( created )
            sort_area( tarea, TRUE );
    }
}

void do_aassign( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    AREA_DATA              *tarea,
                           *tmp;

    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' ) {
        send_to_char( "Syntax: aassign <filename.are>\r\n", ch );
        return;
    }

    if ( !str_cmp( "none", argument ) || !str_cmp( "null", argument )
         || !str_cmp( "clear", argument ) ) {
        ch->pcdata->area = NULL;
        assign_area( ch, FALSE );
        if ( !ch->pcdata->area )
            send_to_char( "Area pointer cleared.\r\n", ch );
        else
            send_to_char( "Originally assigned area restored.\r\n", ch );
        return;
    }

    snprintf( buf, MSL, "%s", argument );
    tarea = NULL;

    if ( get_trust( ch ) >= LEVEL_AJ_CPL
         || ( is_name( buf, ch->pcdata->bestowments )
              && get_trust( ch ) >= sysdata.level_modify_proto ) )
        for ( tmp = first_area; tmp; tmp = tmp->next )
            if ( !str_cmp( buf, tmp->filename ) ) {
                tarea = tmp;
                break;
            }

    if ( ( !str_cmp( argument, "tutorial.are" ) && ch->level < 108 )
         || ( !str_cmp( argument, "etutorial.are" ) && ch->level < 108 )
         || ( !str_cmp( argument, "dtutorial.are" ) && ch->level < 108 ) ) {
        send_to_char( "You need Vladaar's permission to change anything with tutorials.\r\n", ch );
        return;
    }

    if ( !tarea )
        for ( tmp = first_build; tmp; tmp = tmp->next )
            if ( !str_cmp( buf, tmp->filename ) ) {
                if ( get_trust( ch ) >= LEVEL_AJ_CPL
                     || is_name( tmp->filename, ch->pcdata->bestowments ) || ( ch->pcdata->council
                                                                               &&
                                                                               is_name( "aassign",
                                                                                        ch->pcdata->
                                                                                        council->
                                                                                        powers ) ) )
                {
                    tarea = tmp;
                    break;
                }
                else {
                    send_to_char( "You do not have permission to use that area.\r\n", ch );
                    return;
                }
            }

    if ( !tarea ) {
        if ( get_trust( ch ) >= sysdata.level_modify_proto )
            send_to_char( "No such area.  Use 'zones'.\r\n", ch );
        else
            send_to_char( "No such area.  Use 'newzones'.\r\n", ch );
        return;
    }
    ch->pcdata->area = tarea;
    ch_printf( ch, "Assigning you: %s\r\n", tarea->name );
    return;
}

EXTRA_DESCR_DATA       *SetRExtra( ROOM_INDEX_DATA *room, char *keywords )
{
    EXTRA_DESCR_DATA       *ed;

    for ( ed = room->first_extradesc; ed; ed = ed->next ) {
        if ( is_name( keywords, ed->keyword ) )
            break;
    }
    if ( !ed ) {
        CREATE( ed, EXTRA_DESCR_DATA, 1 );

        LINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
        ed->keyword = STRALLOC( keywords );
        top_ed++;
    }
    return ed;
}

bool DelRExtra( ROOM_INDEX_DATA *room, char *keywords )
{
    EXTRA_DESCR_DATA       *rmed;

    for ( rmed = room->first_extradesc; rmed; rmed = rmed->next ) {
        if ( is_name( keywords, rmed->keyword ) )
            break;
    }
    if ( !rmed )
        return FALSE;
    UNLINK( rmed, room->first_extradesc, room->last_extradesc, next, prev );
    STRFREE( rmed->keyword );
    STRFREE( rmed->description );
    DISPOSE( rmed );
    top_ed--;
    return TRUE;
}

EXTRA_DESCR_DATA       *SetOExtra( OBJ_DATA *obj, char *keywords )
{
    EXTRA_DESCR_DATA       *ed;

    for ( ed = obj->first_extradesc; ed; ed = ed->next ) {
        if ( is_name( keywords, ed->keyword ) )
            break;
    }
    if ( !ed ) {
        CREATE( ed, EXTRA_DESCR_DATA, 1 );

        LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
        ed->keyword = STRALLOC( keywords );
        top_ed++;
    }
    return ed;
}

bool DelOExtra( OBJ_DATA *obj, char *keywords )
{
    EXTRA_DESCR_DATA       *rmed;

    for ( rmed = obj->first_extradesc; rmed; rmed = rmed->next ) {
        if ( is_name( keywords, rmed->keyword ) )
            break;
    }
    if ( !rmed )
        return FALSE;
    UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
    STRFREE( rmed->keyword );
    STRFREE( rmed->description );
    DISPOSE( rmed );
    top_ed--;
    return TRUE;
}

EXTRA_DESCR_DATA       *SetOExtraProto( OBJ_INDEX_DATA *obj, char *keywords )
{
    EXTRA_DESCR_DATA       *ed;

    for ( ed = obj->first_extradesc; ed; ed = ed->next ) {
        if ( is_name( keywords, ed->keyword ) )
            break;
    }
    if ( !ed ) {
        CREATE( ed, EXTRA_DESCR_DATA, 1 );

        LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
        ed->keyword = STRALLOC( keywords );
        top_ed++;
    }
    return ed;
}

bool DelOExtraProto( OBJ_INDEX_DATA *obj, char *keywords )
{
    EXTRA_DESCR_DATA       *rmed;

    for ( rmed = obj->first_extradesc; rmed; rmed = rmed->next ) {
        if ( is_name( keywords, rmed->keyword ) )
            break;
    }
    if ( !rmed )
        return FALSE;
    UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
    STRFREE( rmed->keyword );
    STRFREE( rmed->description );
    DISPOSE( rmed );
    top_ed--;
    return TRUE;
}

void fold_area( AREA_DATA *tarea, char *filename, bool install )
{
    RESET_DATA             *pReset,
                           *tReset,
                           *gReset;
    ROOM_INDEX_DATA        *room;
    MOB_INDEX_DATA         *pMobIndex;
    OBJ_INDEX_DATA         *pObjIndex;
    MPROG_DATA             *mprog;
    EXIT_DATA              *xit;
    EXTRA_DESCR_DATA       *ed;
    AFFECT_DATA            *paf;
    SHOP_DATA              *pShop;
    REPAIR_DATA            *pRepair;
    char                    buf[MSL];
    FILE                   *fpout;
    int                     vnum;
    int                     val0,
                            val1,
                            val2,
                            val3,
                            val4,
                            val5,
                            val6;
    bool                    complexmob;

    snprintf( buf, MSL, "Saving %s...", tarea->filename );
    log_string_plus( buf, LOG_NORMAL, LEVEL_AJ_SGT );
    snprintf( buf, MSL, "%s.bak", filename );
    remove( buf );
    rename( filename, buf );
    if ( ( fpout = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "fold_area: FileOpen" );
        perror( filename );
        return;
    }
    if ( install )
        REMOVE_BIT( tarea->flags, AFLAG_PROTOTYPE );
    fprintf( fpout, "#AREA        %s~\n\n\n\n", !tarea->name ? "" : tarea->name );
    fprintf( fpout, "#VERSION     %d\n", AREA_VERSION_WRITE );
    fprintf( fpout, "#AUTHOR      %s~\n", !tarea->author ? "" : tarea->author );
    fprintf( fpout, "#DERIVATIVES %s~\n", !tarea->derivatives ? "" : tarea->derivatives );
    fprintf( fpout, "#COLOR       %d\n", tarea->color );
    fprintf( fpout, "#HTOWN       %s~\n", !tarea->htown ? "" : tarea->htown );
    fprintf( fpout, "#DESC        %s~\n\n", !tarea->desc ? "" : tarea->desc );
    fprintf( fpout, "#RANGES\n" );
    fprintf( fpout, "%d %d %d %d\n", tarea->low_soft_range, tarea->hi_soft_range,
             tarea->low_hard_range, tarea->hi_hard_range );
    fprintf( fpout, "$\n\n" );
    if ( AREA_VERSION_WRITE > 1 )
        fprintf( fpout, "#SPELLLIMIT %d\n", tarea->spelllimit );
    if ( tarea->resetmsg )                             /* Rennard */
        fprintf( fpout, "#WEATHERCELL %d %d\n\n", tarea->weatherx, tarea->weathery );
    fprintf( fpout, "#RESETMSG %s~\n\n", !tarea->resetmsg ? "" : tarea->resetmsg );
    if ( tarea->reset_frequency )
        fprintf( fpout, "#FLAGS\n%d %d\n\n", tarea->flags, tarea->reset_frequency );
    else
        fprintf( fpout, "#FLAGS\n%d\n\n", tarea->flags );
    fprintf( fpout, "#CURRENCY %d\n\n", tarea->currvnum );
    fprintf( fpout, "#HIGHECONOMY" );
    for ( vnum = 0; vnum < MAX_CURR_TYPE; vnum++ )
        fprintf( fpout, " %d", tarea->high_economy[vnum] );
    fprintf( fpout, " -1\n\n" );
    fprintf( fpout, "#LOWECONOMY" );
    for ( vnum = 0; vnum < MAX_CURR_TYPE; vnum++ )
        fprintf( fpout, " %d", tarea->low_economy[vnum] );
    fprintf( fpout, " -1\n\n" );
// influence code for areas
    if ( AREA_VERSION_WRITE > 5 ) {
        fprintf( fpout, "#CLANNAME %s~\n\n", tarea->clanname );
        fprintf( fpout, "#INFLUENCE %d\n\n", tarea->influence );
    }
    /*
     * save mobiles 
     */
    fprintf( fpout, "#MOBILES\n" );
    for ( vnum = tarea->low_m_vnum; vnum <= tarea->hi_m_vnum; vnum++ ) {
        if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
            continue;
        if ( install )
            xREMOVE_BIT( pMobIndex->act, ACT_PROTOTYPE );
        if ( pMobIndex->perm_str != 13 || pMobIndex->perm_int != 13
             || pMobIndex->perm_wis != 13 || pMobIndex->perm_dex != 13
             || pMobIndex->perm_con != 13 || pMobIndex->perm_cha != 13
             || pMobIndex->perm_lck != 13
             || pMobIndex->hitroll != 0 || pMobIndex->damroll != 0
             || pMobIndex->race != 0 || pMobIndex->Class != 3
             || !xIS_EMPTY( pMobIndex->attacks ) || !xIS_EMPTY( pMobIndex->defenses )
             || pMobIndex->height != 0 || pMobIndex->weight != 0 || pMobIndex->speaks != 0
             || pMobIndex->speaking != 0 || !xIS_EMPTY( pMobIndex->xflags )
             || pMobIndex->numattacks != 0 )
            complexmob = TRUE;
        else
            complexmob = FALSE;
        fprintf( fpout, "#%d\n", vnum );

        if ( !pMobIndex->player_name )
            bug( "%s: mobile %d has a NULL name and should be set to keep from causing issues later.", __FUNCTION__, pMobIndex->vnum );

        fprintf( fpout, "%s~\n", !pMobIndex->player_name ? "" : pMobIndex->player_name );
        fprintf( fpout, "%s~\n", !pMobIndex->short_descr ? "" : pMobIndex->short_descr );
        fprintf( fpout, "%s~\n", !pMobIndex->long_descr ? "" : strip_cr( pMobIndex->long_descr ) );
        fprintf( fpout, "%s~\n",
                 !pMobIndex->description ? "" : strip_cr( pMobIndex->description ) );
        if ( AREA_VERSION_WRITE > 4 )
            fprintf( fpout, "%d\n", pMobIndex->color );
// influence code for areas
        if ( AREA_VERSION_WRITE > 5 ) {
            fprintf( fpout, "%s~\n", !pMobIndex->clanname ? "clanname none" : pMobIndex->clanname );
            fprintf( fpout, "%d\n", pMobIndex->influence );
        }
        if ( AREA_VERSION_WRITE > 6 )
            fprintf( fpout, "%d\n", pMobIndex->slicevnum );
        fprintf( fpout, "%s ", print_bitvector( &pMobIndex->act ) );
        fprintf( fpout, "%s %d %c\n", print_bitvector( &pMobIndex->affected_by ),
                 pMobIndex->alignment, complexmob ? 'C' : 'S' );
        fprintf( fpout, "%d %d %d ", pMobIndex->level, pMobIndex->mobthac0, pMobIndex->ac );
        fprintf( fpout, "%dd%d+%d ", pMobIndex->hitnodice, pMobIndex->hitsizedice,
                 pMobIndex->hitplus );
        fprintf( fpout, "%dd%d+%d\n", pMobIndex->damnodice, pMobIndex->damsizedice,
                 pMobIndex->damplus );
        for ( val0 = 0; val0 < MAX_CURR_TYPE; val0++ )
            fprintf( fpout, "%d ", GET_MONEY( pMobIndex, val0 ) );
        fprintf( fpout, "\n" );
        fprintf( fpout, "%d\n", pMobIndex->exp );
        /*
         * Need to convert to new positions correctly on loadup sigh -Shaddai 
         */
        fprintf( fpout, "%d %d %d\n", pMobIndex->position + 100, pMobIndex->defposition + 100,
                 pMobIndex->sex );
        if ( complexmob ) {
            fprintf( fpout, "%d %d %d %d %d %d %d\n", pMobIndex->perm_str, pMobIndex->perm_int,
                     pMobIndex->perm_wis, pMobIndex->perm_dex, pMobIndex->perm_con,
                     pMobIndex->perm_cha, pMobIndex->perm_lck );
            fprintf( fpout, "%d %d %d %d %d\n", pMobIndex->saving_poison_death,
                     pMobIndex->saving_wand, pMobIndex->saving_para_petri, pMobIndex->saving_breath,
                     pMobIndex->saving_spell_staff );
            fprintf( fpout, "%d %d %d %d %d %d %d\n", pMobIndex->race, pMobIndex->Class,
                     pMobIndex->height, pMobIndex->weight, pMobIndex->speaks, pMobIndex->speaking,
                     pMobIndex->numattacks );
            fprintf( fpout, "%d %d %s %d %d %d ", pMobIndex->hitroll, pMobIndex->damroll,
                     print_bitvector( &pMobIndex->xflags ), pMobIndex->resistant, pMobIndex->immune,
                     pMobIndex->susceptible );
            fprintf( fpout, "%s ", print_bitvector( &pMobIndex->attacks ) );
            fprintf( fpout, "%s\n", print_bitvector( &pMobIndex->defenses ) );
        }
        if ( pMobIndex->mudprogs ) {
            for ( mprog = pMobIndex->mudprogs; mprog; mprog = mprog->next )
                fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                         mprog->arglist, strip_cr( mprog->comlist ) );
            fprintf( fpout, "|\n" );
        }
    }
    fprintf( fpout, "#0\n\n\n" );
    if ( install && vnum < tarea->hi_m_vnum )
        tarea->hi_m_vnum = vnum - 1;
    /*
     * save objects 
     */
    fprintf( fpout, "#OBJECTS\n" );
    for ( vnum = tarea->low_o_vnum; vnum <= tarea->hi_o_vnum; vnum++ ) {
        if ( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
            continue;
        if ( install )
            xREMOVE_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
        fprintf( fpout, "#%d\n", vnum );
        fprintf( fpout, "%s~\n", !pObjIndex->name ? "" : pObjIndex->name );
        fprintf( fpout, "%s~\n", !pObjIndex->short_descr ? "" : pObjIndex->short_descr );
        fprintf( fpout, "%s~\n", !pObjIndex->description ? "" : pObjIndex->description );
        fprintf( fpout, "%s~\n", !pObjIndex->action_desc ? "" : pObjIndex->action_desc );
        if ( pObjIndex->layers )
            fprintf( fpout, "%d %s %d %d\n", pObjIndex->item_type,
                     print_bitvector( &pObjIndex->extra_flags ), pObjIndex->wear_flags,
                     pObjIndex->layers );
        else
            fprintf( fpout, "%d %s %d\n", pObjIndex->item_type,
                     print_bitvector( &pObjIndex->extra_flags ), pObjIndex->wear_flags );
        val0 = pObjIndex->value[0];
        val1 = pObjIndex->value[1];
        val2 = pObjIndex->value[2];
        val3 = pObjIndex->value[3];
        val4 = pObjIndex->value[4];
        val5 = pObjIndex->value[5];
        val6 = pObjIndex->value[6];
        switch ( pObjIndex->item_type ) {
            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
                if ( IS_VALID_SN( val1 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val1 = skill_table[val1]->slot;
                    else
                        val1 = HAS_SPELL_INDEX;
                }
                if ( IS_VALID_SN( val2 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val2 = skill_table[val2]->slot;
                    else
                        val2 = HAS_SPELL_INDEX;
                }
                if ( IS_VALID_SN( val3 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val3 = skill_table[val3]->slot;
                    else
                        val3 = HAS_SPELL_INDEX;
                }
                break;
            case ITEM_STAFF:
            case ITEM_WAND:
                if ( IS_VALID_SN( val3 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val3 = skill_table[val3]->slot;
                    else
                        val3 = HAS_SPELL_INDEX;
                }
                break;
            case ITEM_SALVE:
                if ( IS_VALID_SN( val4 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val4 = skill_table[val4]->slot;
                    else
                        val4 = HAS_SPELL_INDEX;
                }
                if ( IS_VALID_SN( val6 ) ) {
                    if ( AREA_VERSION_WRITE == 0 )
                        val6 = skill_table[val6]->slot;
                    else
                        val6 = HAS_SPELL_INDEX;
                }
                break;
        }
        fprintf( fpout, "%d %d %d %d %d %d %d\n", val0, val1, val2, val3, val4, val5, val6 );
        /*
         * Currency problem 
         */
        fprintf( fpout, "%d %d %d\n", pObjIndex->weight, pObjIndex->cost,
                 pObjIndex->rent ? pObjIndex->rent : ( int ) ( pObjIndex->cost / 10 ) );
        if ( AREA_VERSION_WRITE > 1 )
            fprintf( fpout, "%d\n", pObjIndex->currtype );
        if ( AREA_VERSION_WRITE > 3 )
            fprintf( fpout, "%d\n", pObjIndex->color );
        if ( AREA_VERSION_WRITE > 0 )
            switch ( pObjIndex->item_type ) {
                case ITEM_PILL:
                case ITEM_POTION:
                case ITEM_SCROLL:
                    fprintf( fpout, "'%s' '%s' '%s'\n",
                             IS_VALID_SN( pObjIndex->
                                          value[1] ) ? skill_table[pObjIndex->value[1]]->name :
                             "NONE",
                             IS_VALID_SN( pObjIndex->value[2] ) ? skill_table[pObjIndex->value[2]]->
                             name : "NONE",
                             IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->
                             name : "NONE" );
                    break;
                case ITEM_STAFF:
                case ITEM_WAND:
                    fprintf( fpout, "'%s'\n",
                             IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->
                             name : "NONE" );
                    break;
                case ITEM_SALVE:
                    fprintf( fpout, "'%s' '%s'\n",
                             IS_VALID_SN( pObjIndex->value[4] ) ? skill_table[pObjIndex->value[4]]->
                             name : "NONE",
                             IS_VALID_SN( pObjIndex->value[6] ) ? skill_table[pObjIndex->value[6]]->
                             name : "NONE" );
                    break;
            }

//    if(pObjIndex->rating && pObjIndex->rating > 0)
        fprintf( fpout, "S %d %d\n", pObjIndex->size, pObjIndex->rating );

//    else
//      fprintf(fpout, "S %d\n", pObjIndex->size);

        for ( ed = pObjIndex->first_extradesc; ed; ed = ed->next )
            fprintf( fpout, "E\n%s~\n%s~\n", ed->keyword, strip_cr( ed->description ) );
        for ( paf = pObjIndex->first_affect; paf; paf = paf->next )
            fprintf( fpout, "A\n%d %d\n", paf->location,
                     ( ( paf->location == APPLY_WEAPONSPELL
                         || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_REMOVESPELL
                         || paf->location == APPLY_STRIPSN
                         || paf->location == APPLY_RECURRINGSPELL )
                       && IS_VALID_SN( paf->modifier ) ) ? skill_table[paf->modifier]->slot : paf->
                     modifier );
        if ( pObjIndex->mudprogs ) {
            for ( mprog = pObjIndex->mudprogs; mprog; mprog = mprog->next )
                fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                         mprog->arglist, strip_cr( mprog->comlist ) );
            fprintf( fpout, "|\n" );
        }
    }
    fprintf( fpout, "#0\n\n\n" );
    if ( install && vnum < tarea->hi_o_vnum )
        tarea->hi_o_vnum = vnum - 1;
    /*
     * save rooms   
     */
    fprintf( fpout, "#ROOMS\n" );
    for ( vnum = tarea->low_r_vnum; vnum <= tarea->hi_r_vnum; vnum++ ) {
        if ( ( room = get_room_index( vnum ) ) == NULL )
            continue;
        if ( install ) {
            CHAR_DATA              *victim,
                                   *vnext;
            OBJ_DATA               *obj,
                                   *obj_next;

            /*
             * remove prototype flag from room 
             */
            REMOVE_BIT( room->room_flags, ROOM_PROTOTYPE );
            /*
             * purge room of (prototyped) mobiles 
             */
            for ( victim = room->first_person; victim; victim = vnext ) {
                vnext = victim->next_in_room;
                if ( IS_NPC( victim ) )
                    extract_char( victim, TRUE );
            }
            /*
             * purge room of (prototyped) objects 
             */
            for ( obj = room->first_content; obj; obj = obj_next ) {
                obj_next = obj->next_content;
                extract_obj( obj );
            }
        }
        fprintf( fpout, "#%d\n", vnum );
        fprintf( fpout, "%s~\n", !room->name ? "" : room->name );
        fprintf( fpout, "%s~\n", !room->description ? "" : strip_cr( room->description ) );
        if ( ( room->tele_delay > 0 && room->tele_vnum > 0 ) || room->tunnel > 0
             || room->height > 0 )
            fprintf( fpout, "1 %d %d %d %d %d %d\n", room->room_flags, room->sector_type,
                     room->tele_delay, room->tele_vnum, room->tunnel, room->height );
        else
            fprintf( fpout, "1 %d %d\n", room->room_flags, room->sector_type );

/* Trades before exits! */
//    fprintf(fpout, "%d %d\n", room->rarity, room->maxresources);

        for ( xit = room->first_exit; xit; xit = xit->next ) {
            if ( IS_SET( xit->exit_info, EX_PORTAL ) ) /* don't fold portals */
                continue;
            fprintf( fpout, "D%d\n", xit->vdir );
            fprintf( fpout, "%s~\n", !xit->description ? "" : strip_cr( xit->description ) );
            fprintf( fpout, "%s~\n", !xit->keyword ? "" : strip_cr( xit->keyword ) );
            if ( xit->pull )
                fprintf( fpout, "%d %d %d %d %d\n", xit->exit_info & ~EX_BASHED, xit->key,
                         xit->vnum, xit->pulltype, xit->pull );
            else
                fprintf( fpout, "%d %d %d\n", xit->exit_info & ~EX_BASHED, xit->key, xit->vnum );
        }
        for ( pReset = room->first_reset; pReset; pReset = pReset->next ) {
            switch ( pReset->command ) {               /* extra arg1 arg2 arg3 */
                default:
                case '*':
                    break;
                case 'm':
                case 'M':
                case 'o':
                case 'O':
                    fprintf( fpout, "R %c %d %d %d %d\n", UPPER( pReset->command ), pReset->extra,
                             pReset->arg1, pReset->arg2, pReset->arg3 );

                    for ( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset ) {
                        switch ( tReset->command ) {
                            case 'p':
                            case 'P':
                            case 'e':
                            case 'E':
                                fprintf( fpout, "  R %c %d %d %d %d\n", UPPER( tReset->command ),
                                         tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3 );
                                if ( tReset->first_reset ) {
                                    for ( gReset = tReset->first_reset; gReset;
                                          gReset = gReset->next_reset ) {
                                        if ( gReset->command != 'p' && gReset->command != 'P' )
                                            continue;
                                        fprintf( fpout, "    R %c %d %d %d %d\n",
                                                 UPPER( gReset->command ), gReset->extra,
                                                 gReset->arg1, gReset->arg2, gReset->arg3 );
                                    }
                                }
                                break;

                            case 'g':
                            case 'G':
                                fprintf( fpout, "  R %c %d %d %d\n", UPPER( tReset->command ),
                                         tReset->extra, tReset->arg1, tReset->arg2 );
                                if ( tReset->first_reset ) {
                                    for ( gReset = tReset->first_reset; gReset;
                                          gReset = gReset->next_reset ) {
                                        if ( gReset->command != 'p' && gReset->command != 'P' )
                                            continue;
                                        fprintf( fpout, "    R %c %d %d %d %d\n",
                                                 UPPER( gReset->command ), gReset->extra,
                                                 gReset->arg1, gReset->arg2, gReset->arg3 );
                                    }
                                }
                                break;

                            case 't':
                            case 'T':
                            case 'h':
                            case 'H':
                                fprintf( fpout, "  R %c %d %d %d %d\n", UPPER( tReset->command ),
                                         tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3 );
                                break;
                        }
                    }
                    break;

                case 'd':
                case 'D':
                case 't':
                case 'T':
                case 'h':
                case 'H':
                    fprintf( fpout, "R %c %d %d %d %d\n", UPPER( pReset->command ), pReset->extra,
                             pReset->arg1, pReset->arg2, pReset->arg3 );
                    break;

                case 'r':
                case 'R':
                    fprintf( fpout, "R %c %d %d %d\n", UPPER( pReset->command ), pReset->extra,
                             pReset->arg1, pReset->arg2 );
                    break;
            }
        }
        for ( ed = room->first_extradesc; ed; ed = ed->next )
            fprintf( fpout, "E\n%s~\n%s~\n", !ed->keyword ? "" : ed->keyword,
                     !ed->description ? "" : strip_cr( ed->description ) );
        if ( room->mudprogs ) {
            for ( mprog = room->mudprogs; mprog; mprog = mprog->next )
                fprintf( fpout, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
                         mprog->arglist, strip_cr( mprog->comlist ) );
            fprintf( fpout, "|\n" );
        }
        fprintf( fpout, "S\n" );
    }
    fprintf( fpout, "#0\n\n\n" );
    if ( install && vnum < tarea->hi_r_vnum )
        tarea->hi_r_vnum = vnum - 1;

    /*
     * save shops 
     */
    fprintf( fpout, "#SHOPS\n" );
    for ( vnum = tarea->low_m_vnum; vnum <= tarea->hi_m_vnum; vnum++ ) {
        if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
            continue;
        if ( ( pShop = pMobIndex->pShop ) == NULL )
            continue;
        fprintf( fpout, " %d   %2d %2d %2d %2d %2d   %3d %3d",
                 pShop->keeper, pShop->buy_type[0], pShop->buy_type[1], pShop->buy_type[2],
                 pShop->buy_type[3], pShop->buy_type[4], pShop->profit_buy, pShop->profit_sell );
        fprintf( fpout, "        %2d %2d    ; %s\n", pShop->open_hour, pShop->close_hour,
                 pMobIndex->short_descr );
    }
    fprintf( fpout, "0\n\n\n" );
    /*
     * save repair shops 
     */
    fprintf( fpout, "#REPAIRS\n" );
    for ( vnum = tarea->low_m_vnum; vnum <= tarea->hi_m_vnum; vnum++ ) {
        if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
            continue;
        if ( ( pRepair = pMobIndex->rShop ) == NULL )
            continue;
        fprintf( fpout, " %d   %2d %2d %2d         %3d %3d", pRepair->keeper, pRepair->fix_type[0],
                 pRepair->fix_type[1], pRepair->fix_type[2], pRepair->profit_fix,
                 pRepair->shop_type );
        fprintf( fpout, "        %2d %2d    ; %s\n", pRepair->open_hour, pRepair->close_hour,
                 pMobIndex->short_descr );
    }
    fprintf( fpout, "0\n\n\n" );
    /*
     * save specials 
     */
    fprintf( fpout, "#SPECIALS\n" );
    for ( vnum = tarea->low_m_vnum; vnum <= tarea->hi_m_vnum; vnum++ ) {
        if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
            continue;
        if ( !pMobIndex->spec_fun )
            continue;
        fprintf( fpout, "M  %d %s\n", pMobIndex->vnum, lookup_spec( pMobIndex->spec_fun ) );
    }
    fprintf( fpout, "S\n\n\n" );
    /*
     * END 
     */
    fprintf( fpout, "#$\n" );
    FileClose( fpout );
    return;
}

void do_silentsave( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea,
                           *installed;
    char                    filename[256];

    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL || !ch->pcdata
         || ( argument[0] == '\0' && !ch->pcdata->area ) ) {
        return;
    }

    if ( argument[0] == '\0' )
        tarea = ch->pcdata->area;
    else {
        bool                    found;

        if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
            return;
        }
        for ( found = FALSE, tarea = first_build; tarea; tarea = tarea->next )
            if ( !str_cmp( tarea->filename, argument ) ) {
                found = TRUE;
                break;
            }
        if ( !found ) {
            return;
        }
    }

    if ( !tarea ) {
        return;
    }

    /*
     * Ensure not wiping out their area with save before load - Scryn 8/11 
     */
    if ( !IS_SET( tarea->status, AREA_LOADED ) ) {
        return;
    }
    for ( installed = first_area; installed; installed = installed->next ) {
        if ( !str_cmp( tarea->filename, installed->filename ) ) {
            return;
        }
    }

    snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
    fold_area( tarea, filename, FALSE );
    set_char_color( AT_IMMORT, ch );
}

void do_savearea( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea,
                           *installed;
    char                    filename[256];

    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL || !ch->pcdata
         || ( argument[0] == '\0' && !ch->pcdata->area ) ) {
        send_to_char( "You don't have an assigned area to save.\r\n", ch );
        return;
    }

    if ( argument[0] == '\0' )
        tarea = ch->pcdata->area;
    else {
        bool                    found;

        if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "You can only save your own area.\r\n", ch );
            return;
        }
        for ( found = FALSE, tarea = first_build; tarea; tarea = tarea->next )
            if ( !str_cmp( tarea->filename, argument ) ) {
                found = TRUE;
                break;
            }
        if ( !found ) {
            send_to_char( "Area not found.\r\n", ch );
            return;
        }
    }

    if ( !tarea ) {
        send_to_char( "No area to save.\r\n", ch );
        return;
    }

    /*
     * Ensure not wiping out their area with save before load - Scryn 8/11 
     */
    if ( !IS_SET( tarea->status, AREA_LOADED ) ) {
        send_to_char( "Your area is not loaded!\r\n", ch );
        return;
    }

    for ( installed = first_area; installed; installed = installed->next ) {
        if ( !str_cmp( tarea->filename, installed->filename ) ) {
            send_to_char( "That area is installed and has to be folded not saved!\r\n", ch );
            return;
        }
    }

    snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
    send_to_char( "Saving area...\r\n", ch );
    fold_area( tarea, filename, FALSE );
    set_char_color( AT_IMMORT, ch );
    send_to_char( "Done.\r\n", ch );
}

void do_foldarea( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea;
    char                    Newfilename[MSL];
    bool                    found = FALSE,
        all = FALSE;

    if ( !ch || IS_NPC( ch ) )
        return;

    set_char_color( AT_IMMORT, ch );
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Syntax: folda <area filename>\r\n", ch );
        if ( ch->level > 106 ) {
            send_to_char( "Syntax: folda all\r\n", ch );
        }
        return;
    }

    if ( ( !str_cmp( argument, "tutorial.are" ) && ch->level < 108 )
         || ( !str_cmp( argument, "etutorial.are" ) && ch->level < 108 )
         || ( !str_cmp( argument, "dtutorial.are" ) && ch->level < 108 ) ) {
        send_to_char( "You need Vladaar's permission to change anything with tutorials.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "all" ) ) {
        all = TRUE;
        send_to_char( "Folding all areas...\r\n", ch );
    }
    else
        send_to_char( "Folding area...\r\n", ch );
    for ( tarea = first_area; tarea; tarea = tarea->next ) {
        if ( all || !str_cmp( tarea->filename, argument ) ) {
            snprintf( Newfilename, MSL, "%s%s", AREA_DIR, tarea->filename );
            fold_area( tarea, Newfilename, FALSE );
            if ( found == FALSE )
                found = TRUE;
        }
    }
    set_char_color( AT_IMMORT, ch );
    if ( found == FALSE )
        send_to_char( "No such area exists.\r\n", ch );
    else
        send_to_char( "Folding complete.\r\n", ch );
}

extern int              top_area;

void write_area_list(  )
{
    AREA_DATA              *tarea;
    FILE                   *fpout;

    fpout = FileOpen( AREA_LIST, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!", AREA_LIST );
        return;
    }
    fprintf( fpout, "help.are\n" );
    for ( tarea = first_area; tarea; tarea = tarea->next )
        fprintf( fpout, "%s\n", tarea->filename );
    fprintf( fpout, "$\n" );
    FileClose( fpout );
}

void do_installarea( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea;
    char                    filename[256];
    char                    arg[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    int                     num;
    DESCRIPTOR_DATA        *d;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: installarea <filename> [Area title]\r\n", ch );
        return;
    }

    for ( tarea = first_build; tarea; tarea = tarea->next ) {
        if ( !str_cmp( tarea->filename, arg ) ) {
            if ( argument && argument[0] != '\0' ) {
                STRFREE( tarea->name );
                tarea->name = STRALLOC( argument );
            }

            /*
             * Fold area with install flag -- auto-removes prototype flags 
             */
            send_to_char( "Saving and installing file...\r\n", ch );

            snprintf( filename, sizeof( filename ), "%s%s", BUILD_DIR, tarea->filename );
            fold_area( tarea, filename, TRUE );

            /*
             * Remove from prototype area list 
             */
            UNLINK( tarea, first_build, last_build, next, prev );

            /*
             * Add to real area list 
             */
            LINK( tarea, first_area, last_area, next, prev );

            /*
             * Fix up author if online 
             */
            for ( d = first_descriptor; d; d = d->next ) {
                if ( d->character && d->character->pcdata && d->character->pcdata->area == tarea ) {
                    /*
                     * remove area from author 
                     */
                    d->character->pcdata->area = NULL;
                    /*
                     * clear out author vnums 
                     */
                    d->character->pcdata->r_range_lo = 0;
                    d->character->pcdata->r_range_hi = 0;
                    d->character->pcdata->o_range_lo = 0;
                    d->character->pcdata->o_range_hi = 0;
                    d->character->pcdata->m_range_lo = 0;
                    d->character->pcdata->m_range_hi = 0;
                }
            }

            top_area++;
            send_to_char( "Writing area.lst...\r\n", ch );
            write_area_list(  );
            send_to_char( "Resetting new area.\r\n", ch );
            num = tarea->nplayer;
            tarea->nplayer = 0;
            reset_area( tarea );
            tarea->nplayer = num;
            send_to_char( "Renaming author's building file.\r\n", ch );
            sprintf( buf, "%s%s.installed", BUILD_DIR, tarea->filename );
            sprintf( arg, "%s%s", BUILD_DIR, tarea->filename );
            rename( arg, buf );
            send_to_char( "Done.\r\n", ch );
            return;
        }
    }
    send_to_char( "No such area exists.\r\n", ch );
}

void do_astat( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea;
    bool                    proto,
                            found;
    int                     pdeaths = 0,
        pkills = 0,
        mdeaths = 0,
        mkills = 0;

    found = FALSE;
    proto = FALSE;

    set_char_color( AT_PLAIN, ch );

    if ( !str_cmp( "summary", argument ) ) {
        for ( tarea = first_area; tarea; tarea = tarea->next ) {
            pdeaths += tarea->pdeaths;
            mdeaths += tarea->mdeaths;
            pkills += tarea->pkills;
            mkills += tarea->mkills;
        }
        ch_printf( ch, "&WTotal pdeaths:      &w%d\r\n", pdeaths );
        ch_printf( ch, "&WTotal pkills:       &w%d\r\n", pkills );
        ch_printf( ch, "&WTotal mdeaths:      &w%d\r\n", mdeaths );
        ch_printf( ch, "&WTotal mkills:       &w%d\r\n", mkills );
        return;
    }

    for ( tarea = first_area; tarea; tarea = tarea->next )
        if ( !str_cmp( tarea->filename, argument ) ) {
            found = TRUE;
            break;
        }

    if ( !found )
        for ( tarea = first_build; tarea; tarea = tarea->next )
            if ( !str_cmp( tarea->filename, argument ) ) {
                found = TRUE;
                proto = TRUE;
                break;
            }

    if ( !found ) {
        if ( argument && argument[0] != '\0' ) {
            send_to_char( "Area not found.  Check 'zones'.\r\n", ch );
            return;
        }
        else {
            tarea = ch->in_room->area;
        }
    }

    ch_printf( ch,
               "\r\n&wName:     &W%s\r\n&wFilename: &W%-20s  &wPrototype: &W%s\r\n&wAuthor:   &W%s\r\n",
               tarea->name, tarea->filename, proto ? "yes" : "no", tarea->author );
    ch_printf( ch, "&wAge: &W%-3d  &wCurrent number of players: &W%-3d  &wMax players: &W%d\r\n",
               tarea->age, tarea->nplayer, tarea->max_players );
    ch_printf_color( ch, "&wWeather: X Coord: &W%-3d  &w Y Coord: &W%-3d\r\n", tarea->weatherx,
                     tarea->weathery );
    ch_printf( ch, "&wColor: &W%-2d\r\n", tarea->color );
    if ( !proto ) {
        if ( tarea->high_economy[DEFAULT_CURR] )
            ch_printf( ch, "Area economy: %d billion and %d gold coins.\r\n",
                       tarea->high_economy[DEFAULT_CURR], tarea->low_economy[DEFAULT_CURR] );
        else
            ch_printf( ch, "Area economy: %d gold coins.\r\n", tarea->low_economy[DEFAULT_CURR] );
        ch_printf( ch, "&wGold Looted:  &W%d\r\n", tarea->looted[DEFAULT_CURR] );
        if ( tarea->clanname )
            ch_printf( ch, "&wClan Name: &W%s  &wInfluence &W%d\r\n", tarea->clanname,
                       tarea->influence );
        ch_printf( ch,
                   "&wMdeaths: &W%d   &wMkills: &W%d   &wPdeaths: &W%d   &wPkills: &W%d   &wIllegalPK: &W%d\r\n",
                   tarea->mdeaths, tarea->mkills, tarea->pdeaths, tarea->pkills,
                   tarea->illegal_pk );
    }
    ch_printf( ch, "&wlow_room: &W%5d    &whi_room: &W%5d\r\n", tarea->low_r_vnum,
               tarea->hi_r_vnum );
    ch_printf( ch, "&wlow_obj : &W%5d    &whi_obj : &W%5d\r\n", tarea->low_o_vnum,
               tarea->hi_o_vnum );
    ch_printf( ch, "&wlow_mob : &W%5d    &whi_mob : &W%5d\r\n", tarea->low_m_vnum,
               tarea->hi_m_vnum );
    ch_printf( ch, "&wsoft range: &W%d - %d    &whard range: &W%d - %d\r\n", tarea->low_soft_range,
               tarea->hi_soft_range, tarea->low_hard_range, tarea->hi_hard_range );
    ch_printf( ch, "&wArea flags: &W%s\r\n", flag_string( tarea->flags, area_flags ) );
    ch_printf( ch, "&wResetmsg: &W%s\r\n", tarea->resetmsg ? tarea->resetmsg : "(default)" );   /* Rennard 
                                                                                                 */
    ch_printf( ch, "&wReset frequency: &W%d &wminutes.\r\n",
               tarea->reset_frequency ? tarea->reset_frequency : 15 );
}

void do_aset( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *tarea;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    bool                    proto,
                            found;
    int                     vnum,
                            value;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    vnum = atoi( argument );

    switch ( ch->substate ) {
        default:
            break;
        case SUB_ROOM_DESC:

            tarea = ( AREA_DATA * ) ch->dest_buf;
            STRFREE( tarea->desc );
            tarea->desc = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Usage: aset <area filename> <field> <value>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( "  low_room hi_room low_obj hi_obj low_mob hi_mob\r\n", ch );
        send_to_char( "  name filename low_soft hi_soft low_hard hi_hard\r\n", ch );
        send_to_char( "  author htown color desc derivatives resetmsg resetfreq flags\r\n", ch );
        send_to_char( "  weatherx weathery\r\n", ch );
        return;
    }

    found = FALSE;
    proto = FALSE;
    for ( tarea = first_area; tarea; tarea = tarea->next )
        if ( !str_cmp( tarea->filename, arg1 ) ) {
            found = TRUE;
            break;
        }

    if ( !found )
        for ( tarea = first_build; tarea; tarea = tarea->next )
            if ( !str_cmp( tarea->filename, arg1 ) ) {
                found = TRUE;
                proto = TRUE;
                break;
            }

    if ( !found ) {
        send_to_char( "Area not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "You can't set an area's name to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( tarea->name ) )
            STRFREE( tarea->name );
        tarea->name = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) ) {
        char                    filename[MSL];

        if ( proto ) {
            send_to_char( "You should only change the filename of installed areas.\r\n", ch );
            return;
        }
        if ( !VLD_STR( argument ) ) {
            send_to_char( "You can't set an area's filename to nothing.\r\n", ch );
            return;
        }
        snprintf( filename, MSL, "%s", tarea->filename );
        if ( VLD_STR( tarea->filename ) )
            STRFREE( tarea->filename );
        tarea->filename = STRALLOC( argument );
        remove( tarea->filename );
        rename( filename, tarea->filename );
        write_area_list(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_economy" ) ) {
        int                     currtype;

        argument = one_argument( argument, arg3 );
        vnum = atoi( arg3 );

        if ( !is_number( argument ) )
            currtype = DEFAULT_CURR;
        else
            currtype = atoi( argument );

        tarea->low_economy[currtype] = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "high_economy" ) ) {
        int                     currtype;

        argument = one_argument( argument, arg3 );
        vnum = atoi( arg3 );

        if ( !is_number( argument ) )
            currtype = DEFAULT_CURR;
        else
            currtype = atoi( argument );

        tarea->high_economy[currtype] = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "weatherx" ) ) {
        tarea->weatherx = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "weathery" ) ) {
        tarea->weathery = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_room" ) ) {
        tarea->low_r_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hi_room" ) ) {
        tarea->hi_r_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_obj" ) ) {
        tarea->low_o_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hi_obj" ) ) {
        tarea->hi_o_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_mob" ) ) {
        tarea->low_m_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hi_mob" ) ) {
        tarea->hi_m_vnum = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_soft" ) ) {
        if ( vnum < 0 || vnum > MAX_LEVEL ) {
            send_to_char( "That is not an acceptable value.\r\n", ch );
            return;
        }

        tarea->low_soft_range = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hi_soft" ) ) {
        if ( vnum < 0 || vnum > MAX_LEVEL ) {
            send_to_char( "That is not an acceptable value.\r\n", ch );
            return;
        }

        tarea->hi_soft_range = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "low_hard" ) ) {
        if ( vnum < 0 || vnum > MAX_LEVEL ) {
            send_to_char( "That is not an acceptable value.\r\n", ch );
            return;
        }

        tarea->low_hard_range = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "hi_hard" ) ) {
        if ( vnum < 0 || vnum > MAX_LEVEL ) {
            send_to_char( "That is not an acceptable value.\r\n", ch );
            return;
        }

        tarea->hi_hard_range = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "author" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Can't set an area's author to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( tarea->author ) )
            STRFREE( tarea->author );
        tarea->author = STRALLOC( argument );
        send_to_char( "Author Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "htown" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Can't set an area's htown to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( tarea->htown ) )
            STRFREE( tarea->htown );
        tarea->htown = STRALLOC( argument );
        send_to_char( "Htown Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "color" ) ) {
        if ( !VLD_STR( argument ) || atoi( argument ) < 0 || atoi( argument ) > 14 ) {
            send_to_char( "Can only set an area's color between 0 and 14.\r\n", ch );
            return;
        }
        tarea->color = atoi( argument );
        send_to_char( "Color set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "desc" ) ) {
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_ROOM_DESC;
        ch->dest_buf = tarea;
        start_editing( ch, tarea->desc );
        return;
    }

    if ( !str_cmp( arg2, "derivatives" ) ) {
        bool                    fDeriv = TRUE;
        char                    new_deriv[MSL];

        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Syntax: aset <filename> derivatives <authors>\r\n", ch );
            return;
        }

        if ( !str_cmp( argument, "none" ) ) {
            if ( VLD_STR( tarea->derivatives ) )
                STRFREE( tarea->derivatives );
            return;
        }

        if ( !tarea->derivatives || tarea->derivatives[0] == '\0' ) {
            fDeriv = FALSE;
        }

        snprintf( new_deriv, MSL, "%s%s%s", fDeriv ? tarea->derivatives : "", fDeriv ? " " : "",
                  argument );

        if ( VLD_STR( tarea->derivatives ) )
            STRFREE( tarea->derivatives );
        tarea->derivatives = STRALLOC( new_deriv );

        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "resetmsg" ) ) {
        if ( tarea->resetmsg )
            STRFREE( tarea->resetmsg );
        if ( VLD_STR( argument ) && str_cmp( argument, "clear" ) )
            tarea->resetmsg = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }                                                  /* Rennard */

    if ( !str_cmp( arg2, "resetfreq" ) ) {
        tarea->reset_frequency = vnum;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Usage: aset <filename> flags <flag> [flag]...\r\n", ch );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_areaflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else {
                if ( IS_SET( tarea->flags, 1 << value ) )
                    REMOVE_BIT( tarea->flags, 1 << value );
                else
                    SET_BIT( tarea->flags, 1 << value );
            }
        }
        return;
    }

    do_aset( ch, ( char * ) "" );
    return;
}

void do_rlist( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room;
    int                     vnum;
    char                    arg1[MIL];
    char                    arg2[MIL];
    AREA_DATA              *tarea;
    int                     lrange;
    int                     trange;

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL || !ch->pcdata
         || ( !ch->pcdata->area && get_trust( ch ) < LEVEL_AJ_SGT ) ) {
        send_to_char( "&YYou don't have an assigned area.\r\n", ch );
        return;
    }

    tarea = ch->in_room->area;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] != '\0' && !is_number( arg1 ) )
        return;
    if ( arg2[0] != '\0' && !is_number( arg2 ) )
        return;

/* I know the above code could be combined into 3 lines, but we write code
   once and then read it many times. It is so much easier to read without
   trying to figure out what opening parenthesis belongs with what closing
   one.  -- Gorog
*/

/* Bah! The following code uses atoi(arg1) and atoi(arg2) without even
   checking they are numeric or even exist. I put a fix for this above.
   It also uses is_number(arg1) without checking if arg1 may be null.
   Caused a crash when the command was uses with a single alpha arg.
   -- Gorog
*/

    if ( tarea ) {
        if ( arg1[0] == '\0' )                         /* cleaned a big scary mess */
            lrange = tarea->low_r_vnum;                /* here.  -Thoric */
        else
            lrange = atoi( arg1 );
        if ( arg2[0] == '\0' )
            trange = tarea->hi_r_vnum;
        else
            trange = atoi( arg2 );

        if ( ( lrange < tarea->low_r_vnum || trange > tarea->hi_r_vnum )
             && get_trust( ch ) < LEVEL_AJ_CPL ) {
            send_to_char( "&YThat is out of your vnum range.\r\n", ch );
            return;
        }
    }
    else {
        lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
        trange = ( is_number( arg2 ) ? atoi( arg2 ) : 1 );
    }

    for ( vnum = lrange; vnum <= trange; vnum++ ) {
        if ( ( room = get_room_index( vnum ) ) == NULL )
            continue;
        pager_printf( ch, "[%5d][Desc %s][Progs %s] %s\r\n", vnum, room->description
                      && strcmp( room->description, "" ) ? "&GY&D" : "&RN&D",
                      room->mudprogs ? "&GY&D" : "&RN&D", room->name );
    }
    return;
}

void do_olist( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA         *obj;
    int                     vnum;
    AREA_DATA              *tarea;
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     lrange;
    int                     trange;

    /*
     * Greater+ can list out of assigned range - Tri (mlist/rlist as well)
     */

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL || !ch->pcdata
         || ( !ch->pcdata->area && get_trust( ch ) < LEVEL_AJ_SGT ) ) {
        send_to_char( "&YYou don't have an assigned area.\r\n", ch );
        return;
    }

    tarea = ch->in_room->area;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( tarea ) {
        if ( arg1[0] == '\0' )                         /* cleaned a big scary mess */
            lrange = tarea->low_o_vnum;                /* here.  -Thoric */
        else
            lrange = atoi( arg1 );
        if ( arg2[0] == '\0' )
            trange = tarea->hi_o_vnum;
        else
            trange = atoi( arg2 );

        if ( ( lrange < tarea->low_o_vnum || trange > tarea->hi_o_vnum )
             && get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "&YThat is out of your vnum range.\r\n", ch );
            return;
        }
    }
    else {
        lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
        trange = ( is_number( arg2 ) ? atoi( arg2 ) : 3 );
    }

    for ( vnum = lrange; vnum <= trange; vnum++ ) {
        if ( ( obj = get_obj_index( vnum ) ) == NULL )
            continue;
        pager_printf( ch, "[%5d][Desc %s][Prog %s] %-20s '%s'\r\n", vnum,
                      obj->first_extradesc ? "&GY&D" : "&RN&D", obj->mudprogs ? "&GY&D" : "&RN&D",
                      obj->name, obj->short_descr );

    }
    return;
}

void do_mlist( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *mob;
    int                     vnum;
    AREA_DATA              *tarea;
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     lrange;
    int                     trange;

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL || !ch->pcdata
         || ( !ch->pcdata->area && get_trust( ch ) < LEVEL_AJ_SGT ) ) {
        send_to_char( "&YYou don't have an assigned area.\r\n", ch );
        return;
    }

    tarea = ch->in_room->area;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( tarea ) {
        if ( arg1[0] == '\0' )                         /* cleaned a big scary mess */
            lrange = tarea->low_m_vnum;                /* here.  -Thoric */
        else
            lrange = atoi( arg1 );
        if ( arg2[0] == '\0' )
            trange = tarea->hi_m_vnum;
        else
            trange = atoi( arg2 );

        if ( ( lrange < tarea->low_m_vnum || trange > tarea->hi_m_vnum )
             && get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "&YThat is out of your vnum range.\r\n", ch );
            return;
        }
    }
    else {
        lrange = ( is_number( arg1 ) ? atoi( arg1 ) : 1 );
        trange = ( is_number( arg2 ) ? atoi( arg2 ) : 1 );
    }

    for ( vnum = lrange; vnum <= trange; vnum++ ) {
        if ( ( mob = get_mob_index( vnum ) ) == NULL )
            continue;
        pager_printf( ch, "[%5d][Lvl %3d][Desc %s][Prog %s] %-20s '%s'\r\n", vnum, mob->level,
                      mob->description ? "&GY&D" : "&RN&D", mob->mudprogs ? "&GY&D" : "&RN&D",
                      mob->player_name, mob->short_descr );
    }
}

void mpedit( CHAR_DATA *ch, MPROG_DATA * mprg, int mptype, char *argument )
{
    if ( mptype != -1 ) {
        mprg->type = mptype;
        if ( mprg->arglist )
            STRFREE( mprg->arglist );
        mprg->arglist = STRALLOC( argument );
    }
    ch->substate = SUB_MPROG_EDIT;
    ch->dest_buf = mprg;
    start_editing( ch, mprg->comlist );
    return;
}

/* Mobprogram editing - cumbersome     -Thoric */
void do_mpedit( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    arg4[MIL];
    CHAR_DATA              *victim;
    MPROG_DATA             *mprog,
                           *mprg,
                           *mprg_next = NULL;
    int                     value,
                            mptype = -1,
        cnt;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't mpedit\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;
        case SUB_MPROG_EDIT:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Thoric.\r\n", ch );
                bug( "do_mpedit: sub_mprog_edit: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            mprog = ( MPROG_DATA * ) ch->dest_buf;
            if ( mprog->comlist )
                STRFREE( mprog->comlist );
            mprog->comlist = copy_buffer( ch );
            stop_editing( ch );
            return;
    }

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    value = atoi( arg3 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: mpedit <victim> <command> [number] <program> <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Command being one of:\r\n", ch );
        send_to_char( "  add delete insert edit list\r\n", ch );
        send_to_char( "Program being one of:\r\n", ch );
        send_to_char( "  act speech rand fight hitprcnt greet allgreet\r\n", ch );
        send_to_char( "  entry give bribe death time hour script \r\n", ch );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
    }
    else {
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "No one like that in all the realms.\r\n", ch );
            return;
        }
    }

    if ( get_trust( ch ) < victim->level || !IS_NPC( victim ) ) {
        send_to_char( "You can't do that!\r\n", ch );
        return;
    }
    if ( get_trust( ch ) < LEVEL_AJ_LT && IS_NPC( victim )
         && xIS_SET( victim->act, ACT_STATSHIELD ) ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Their godly glow prevents you from getting close enough.\r\n", ch );
        return;
    }
    if ( !can_mmodify( ch, victim ) )
        return;

    if ( !xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
        send_to_char( "A mobile must have a prototype flag to be mpset.\r\n", ch );
        return;
    }

    mprog = victim->pIndexData->mudprogs;

    set_char_color( AT_GREEN, ch );

    if ( !str_cmp( arg2, "list" ) ) {
        cnt = 0;
        if ( !mprog ) {
            send_to_char( "That mobile has no mob programs.\r\n", ch );
            return;
        }

        if ( value < 1 ) {
            if ( strcmp( "full", arg3 ) ) {
                for ( mprg = mprog; mprg; mprg = mprg->next ) {
                    ch_printf( ch, "%d>%s %s\r\n", ++cnt, mprog_type_to_name( mprg->type ),
                               mprg->arglist );
                }

                return;
            }
            else {
                for ( mprg = mprog; mprg; mprg = mprg->next ) {
                    ch_printf( ch, "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprg->type ),
                               mprg->arglist, mprg->comlist );
                }

                return;
            }
        }

        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                ch_printf( ch, "%d>%s %s\r\n%s\r\n", cnt, mprog_type_to_name( mprg->type ),
                           mprg->arglist, mprg->comlist );
                break;
            }
        }

        if ( !mprg )
            send_to_char( "Program not found.\r\n", ch );

        return;
    }

    if ( !str_cmp( arg2, "edit" ) ) {
        if ( !mprog ) {
            send_to_char( "That mobile has no mob programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        if ( arg4[0] != '\0' ) {
            mptype = get_mpflag( arg4 );
            if ( mptype == -1 ) {
                send_to_char( "Unknown program type.\r\n", ch );
                return;
            }
        }
        else
            mptype = -1;
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mpedit( ch, mprg, mptype, argument );
                xCLEAR_BITS( victim->pIndexData->progtypes );
                for ( mprg = mprog; mprg; mprg = mprg->next )
                    xSET_BIT( victim->pIndexData->progtypes, mprg->type );
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "delete" ) ) {
        int                     num;
        bool                    found;

        if ( !mprog ) {
            send_to_char( "That mobile has no mob programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        found = FALSE;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mptype = mprg->type;
                found = TRUE;
                break;
            }
        }
        if ( !found ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = num = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next )
            if ( mprg->type == mptype )
                num++;
        if ( value == 1 ) {
            mprg_next = victim->pIndexData->mudprogs;
            victim->pIndexData->mudprogs = mprg_next->next;
        }
        else
            for ( mprg = mprog; mprg; mprg = mprg_next ) {
                mprg_next = mprg->next;
                if ( ++cnt == ( value - 1 ) ) {
                    mprg->next = mprg_next->next;
                    break;
                }
            }
        if ( mprg_next ) {
            if ( mprg_next->arglist )
                STRFREE( mprg_next->arglist );
            if ( mprg_next->comlist )
                STRFREE( mprg_next->comlist );
            DISPOSE( mprg_next );
            if ( num <= 1 )
                xREMOVE_BIT( victim->pIndexData->progtypes, mptype );
            send_to_char( "Program removed.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "insert" ) ) {
        if ( !mprog ) {
            send_to_char( "That mobile has no mob programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        mptype = get_mpflag( arg4 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        if ( value == 1 ) {
            CREATE( mprg, MPROG_DATA, 1 );
            xSET_BIT( victim->pIndexData->progtypes, mptype );
            mpedit( ch, mprg, mptype, argument );
            mprg->next = mprog;
            victim->pIndexData->mudprogs = mprg;
            return;
        }
        cnt = 1;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value && mprg->next ) {
                CREATE( mprg_next, MPROG_DATA, 1 );
                xSET_BIT( victim->pIndexData->progtypes, mptype );
                mpedit( ch, mprg_next, mptype, argument );
                mprg_next->next = mprg->next;
                mprg->next = mprg_next;
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "add" ) ) {
        mptype = get_mpflag( arg3 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( mprog != NULL )
            for ( ; mprog->next; mprog = mprog->next );
        CREATE( mprg, MPROG_DATA, 1 );
        if ( mprog )
            mprog->next = mprg;
        else
            victim->pIndexData->mudprogs = mprg;
        xSET_BIT( victim->pIndexData->progtypes, mptype );
        mpedit( ch, mprg, mptype, argument );
        mprg->next = NULL;
        return;
    }
    do_mpedit( ch, ( char * ) "" );
}

void do_opedit( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    arg4[MIL];
    OBJ_DATA               *obj;
    MPROG_DATA             *mprog,
                           *mprg,
                           *mprg_next = NULL;
    int                     value,
                            mptype = -1,
        cnt;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't opedit\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;
        case SUB_MPROG_EDIT:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Thoric.\r\n", ch );
                bug( "do_opedit: sub_oprog_edit: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            mprog = ( MPROG_DATA * ) ch->dest_buf;
            if ( mprog->comlist )
                STRFREE( mprog->comlist );
            mprog->comlist = copy_buffer( ch );
            stop_editing( ch );
            return;
    }

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    value = atoi( arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: opedit <object> <command> [number] <program> <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Command being one of:\r\n", ch );
        send_to_char( "  add delete insert edit list\r\n", ch );
        send_to_char( "Program being one of:\r\n", ch );
        send_to_char( "  act speech rand wear remove trash zap get\r\n", ch );
        send_to_char( "  drop damage repair greet exa use \r\n", ch );
        send_to_char( "  pull push (for levers,pullchains,buttons)\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Object should be in your inventory to edit.\r\n", ch );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
            send_to_char( "You aren't carrying that.\r\n", ch );
            return;
        }
    }
    else {
        if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "Nothing like that in all the realms.\r\n", ch );
            return;
        }
    }

    if ( !can_omodify( ch, obj ) )
        return;

    if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        send_to_char( "An object must have a prototype flag to be opset.\r\n", ch );
        return;
    }

    mprog = obj->pIndexData->mudprogs;

    set_char_color( AT_GREEN, ch );

    if ( !str_cmp( arg2, "list" ) ) {
        cnt = 0;
        if ( !mprog ) {
            send_to_char( "That object has no obj programs.\r\n", ch );
            return;
        }
        for ( mprg = mprog; mprg; mprg = mprg->next )
            ch_printf( ch, "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprg->type ),
                       mprg->arglist, mprg->comlist );
        return;
    }

    if ( !str_cmp( arg2, "edit" ) ) {
        if ( !mprog ) {
            send_to_char( "That object has no obj programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        if ( arg4[0] != '\0' ) {
            mptype = get_mpflag( arg4 );
            if ( mptype == -1 ) {
                send_to_char( "Unknown program type.\r\n", ch );
                return;
            }
        }
        else
            mptype = -1;
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mpedit( ch, mprg, mptype, argument );
                xCLEAR_BITS( obj->pIndexData->progtypes );
                for ( mprg = mprog; mprg; mprg = mprg->next )
                    xSET_BIT( obj->pIndexData->progtypes, mprg->type );
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "delete" ) ) {
        int                     num;
        bool                    found;

        if ( !mprog ) {
            send_to_char( "That object has no obj programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        found = FALSE;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mptype = mprg->type;
                found = TRUE;
                break;
            }
        }
        if ( !found ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = num = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next )
            if ( mprg->type == mptype )
                num++;
        if ( value == 1 ) {
            mprg_next = obj->pIndexData->mudprogs;
            obj->pIndexData->mudprogs = mprg_next->next;
        }
        else
            for ( mprg = mprog; mprg; mprg = mprg_next ) {
                mprg_next = mprg->next;
                if ( ++cnt == ( value - 1 ) ) {
                    mprg->next = mprg_next->next;
                    break;
                }
            }
        if ( mprg_next ) {
            STRFREE( mprg_next->arglist );
            STRFREE( mprg_next->comlist );
            DISPOSE( mprg_next );
            if ( num <= 1 )
                xREMOVE_BIT( obj->pIndexData->progtypes, mptype );
            send_to_char( "Program removed.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "insert" ) ) {
        if ( !mprog ) {
            send_to_char( "That object has no obj programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg4 );
        mptype = get_mpflag( arg4 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        if ( value == 1 ) {
            CREATE( mprg, MPROG_DATA, 1 );
            xSET_BIT( obj->pIndexData->progtypes, mptype );
            mpedit( ch, mprg, mptype, argument );
            mprg->next = mprog;
            obj->pIndexData->mudprogs = mprg;
            return;
        }
        cnt = 1;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value && mprg->next ) {
                CREATE( mprg_next, MPROG_DATA, 1 );
                xSET_BIT( obj->pIndexData->progtypes, mptype );
                mpedit( ch, mprg_next, mptype, argument );
                mprg_next->next = mprg->next;
                mprg->next = mprg_next;
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "add" ) ) {
        mptype = get_mpflag( arg3 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( mprog != NULL )
            for ( ; mprog->next; mprog = mprog->next );
        CREATE( mprg, MPROG_DATA, 1 );
        if ( mprog )
            mprog->next = mprg;
        else
            obj->pIndexData->mudprogs = mprg;
        xSET_BIT( obj->pIndexData->progtypes, mptype );
        mpedit( ch, mprg, mptype, argument );
        mprg->next = NULL;
        return;
    }
    do_opedit( ch, ( char * ) "" );
}

/*
 * RoomProg Support
 */
void rpedit( CHAR_DATA *ch, MPROG_DATA * mprg, int mptype, char *argument )
{
    if ( mptype != -1 ) {
        mprg->type = mptype;
        if ( mprg->arglist )
            STRFREE( mprg->arglist );
        mprg->arglist = STRALLOC( argument );
    }
    ch->substate = SUB_MPROG_EDIT;
    ch->dest_buf = mprg;
    start_editing( ch, mprg->comlist );
    return;
}

void do_rpedit( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    MPROG_DATA             *mprog,
                           *mprg,
                           *mprg_next = NULL;
    int                     value,
                            mptype = -1,
        cnt;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't rpedit\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;
        case SUB_MPROG_EDIT:
            if ( !ch->dest_buf ) {
                send_to_char( "Fatal error: report to Thoric.\r\n", ch );
                bug( "do_opedit: sub_oprog_edit: NULL ch->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            mprog = ( MPROG_DATA * ) ch->dest_buf;
            if ( mprog->comlist )
                STRFREE( mprog->comlist );
            mprog->comlist = copy_buffer( ch );
            stop_editing( ch );
            return;
    }

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    value = atoi( arg2 );
    /*
     * argument = one_argument(argument, arg3); 
     */

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: rpedit <command> [number] <program> <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Command being one of:\r\n", ch );
        send_to_char( "  add delete insert edit list\r\n", ch );
        send_to_char( "Program being one of:\r\n", ch );
        send_to_char( "  act speech rand sleep rest rfight enter\r\n", ch );
        send_to_char( "  leave death\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "You should be standing in room you wish to edit.\r\n", ch );
        return;
    }

    if ( !can_rmodify( ch, ch->in_room ) )
        return;

    mprog = ch->in_room->mudprogs;

    set_char_color( AT_GREEN, ch );

    if ( !str_cmp( arg1, "list" ) ) {
        cnt = 0;
        if ( !mprog ) {
            send_to_char( "This room has no room programs.\r\n", ch );
            return;
        }
        for ( mprg = mprog; mprg; mprg = mprg->next )
            ch_printf( ch, "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprg->type ),
                       mprg->arglist, mprg->comlist );
        return;
    }

    if ( !str_cmp( arg1, "edit" ) ) {
        if ( !mprog ) {
            send_to_char( "This room has no room programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg3 );
        if ( arg3[0] != '\0' ) {
            mptype = get_mpflag( arg3 );
            if ( mptype == -1 ) {
                send_to_char( "Unknown program type.\r\n", ch );
                return;
            }
        }
        else
            mptype = -1;
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mpedit( ch, mprg, mptype, argument );
                xCLEAR_BITS( ch->in_room->progtypes );
                for ( mprg = mprog; mprg; mprg = mprg->next )
                    xSET_BIT( ch->in_room->progtypes, mprg->type );
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "delete" ) ) {
        int                     num;
        bool                    found;

        if ( !mprog ) {
            send_to_char( "That room has no room programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg3 );
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = 0;
        found = FALSE;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value ) {
                mptype = mprg->type;
                found = TRUE;
                break;
            }
        }
        if ( !found ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        cnt = num = 0;
        for ( mprg = mprog; mprg; mprg = mprg->next )
            if ( mprg->type == mptype )
                num++;
        if ( value == 1 ) {
            mprg_next = ch->in_room->mudprogs;
            ch->in_room->mudprogs = mprg_next->next;
        }
        else
            for ( mprg = mprog; mprg; mprg = mprg_next ) {
                mprg_next = mprg->next;
                if ( ++cnt == ( value - 1 ) ) {
                    mprg->next = mprg_next->next;
                    break;
                }
            }
        if ( mprg_next ) {
            STRFREE( mprg_next->arglist );
            STRFREE( mprg_next->comlist );
            DISPOSE( mprg_next );
            if ( num <= 1 )
                xREMOVE_BIT( ch->in_room->progtypes, mptype );
            send_to_char( "Program removed.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "insert" ) ) {
        if ( !mprog ) {
            send_to_char( "That room has no room programs.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg3 );
        mptype = get_mpflag( arg2 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( value < 1 ) {
            send_to_char( "Program not found.\r\n", ch );
            return;
        }
        if ( value == 1 ) {
            CREATE( mprg, MPROG_DATA, 1 );
            xSET_BIT( ch->in_room->progtypes, mptype );
            mpedit( ch, mprg, mptype, argument );
            mprg->next = mprog;
            ch->in_room->mudprogs = mprg;
            return;
        }
        cnt = 1;
        for ( mprg = mprog; mprg; mprg = mprg->next ) {
            if ( ++cnt == value && mprg->next ) {
                CREATE( mprg_next, MPROG_DATA, 1 );
                xSET_BIT( ch->in_room->progtypes, mptype );
                mpedit( ch, mprg_next, mptype, argument );
                mprg_next->next = mprg->next;
                mprg->next = mprg_next;
                return;
            }
        }
        send_to_char( "Program not found.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "add" ) ) {
        mptype = get_mpflag( arg2 );
        if ( mptype == -1 ) {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
        }
        if ( mprog )
            for ( ; mprog->next; mprog = mprog->next );
        CREATE( mprg, MPROG_DATA, 1 );
        if ( mprog )
            mprog->next = mprg;
        else
            ch->in_room->mudprogs = mprg;
        xSET_BIT( ch->in_room->progtypes, mptype );
        mpedit( ch, mprg, mptype, argument );
        mprg->next = NULL;
        return;
    }
    do_rpedit( ch, ( char * ) "" );
}

void do_rdelete( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    ROOM_INDEX_DATA        *location;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "syntax: rdelte <vnum>\r\n", ch );
        send_to_char( "syntax: rdelete <starting vnum> <ending vnum>\r\n", ch );
        return;
    }
    if ( arg2[0] == '\0' ) {
        /*
         * Find the room.
         */
        if ( ( location = find_location( ch, arg1 ) ) == NULL ) {
            send_to_char( "No such location.\r\n", ch );
            return;
        }

        /*
         * Does the player have the right to delete this room?
         */
        if ( get_trust( ch ) < sysdata.level_modify_proto
             && ( location->vnum < ch->pcdata->r_range_lo
                  || location->vnum > ch->pcdata->r_range_hi ) ) {
            send_to_char( "That room is not in your assigned range.\r\n", ch );
            return;
        }
        /*
         * We could go to the trouble of clearing out the room, but why?
         */
        /*
         * Delete_room does that anyway, but this is probably safer:)
         */
        if ( location->first_person || location->first_content ) {
            send_to_char( "The room must be empty first.\r\n", ch );
            return;
        }

        /*
         * Ok, we've determined that the room exists, it is empty and the
         * player has the authority to delete it, so let's dump the thing.
         * The function to do it is in db.c so it can access the top-room
         * variable.
         */
        delete_room( location );
        fix_exits(  );                                 /* Need to call this to solve a *
                                                        * crash */
        send_to_char( "Room deleted.\r\n", ch );
        return;
    }
    else {
        int                     first_vnum,
                                last_vnum;
        char                    buf[MAX_INPUT_LENGTH];

        if ( !is_number( arg1 ) || !is_number( arg2 ) ) {
            send_to_char( "Both arguments must be a number.\r\n", ch );
            return;
        }

        first_vnum = atoi( arg1 );
        last_vnum = atoi( arg2 );
        for ( int current_vnum = first_vnum; current_vnum <= last_vnum; current_vnum++ ) {
            sprintf( buf, "%d", current_vnum );

            /*
             * Find the room.
             */
            if ( ( location = find_location( ch, buf ) ) == NULL ) {
                ch_printf( ch, "No such location [%s].\r\n", buf );
                continue;
            }

            /*
             * Does the player have the right to delete this room?
             */
            if ( get_trust( ch ) < sysdata.level_modify_proto
                 && ( location->vnum < ch->pcdata->r_range_lo
                      || location->vnum > ch->pcdata->r_range_hi ) ) {
                ch_printf( ch, "That room is not in your assigned range [%s].\r\n", buf );
                continue;
            }
            /*
             * We could go to the trouble of clearing out the room, but why?
             */
            /*
             * Delete_room does that anyway, but this is probably safer:)
             */
            if ( location->first_person || location->first_content ) {
                ch_printf( ch, "The room must be empty first [%s].\r\n", buf );
                continue;
            }
            /*
             * Ok, we've determined that the room exists, it is empty and the
             * player has the authority to delete it, so let's dump the thing.
             * The function to do it is in db.c so it can access the top-room
             * variable.
             */
            delete_room( location );
            ch_printf( ch, "Room deleted [%s].\r\n", buf );
        }

        fix_exits(  );                                 /* Need to call this to solve a *
                                                        * crash */

        return;
    }
}

void do_odelete( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_INDEX_DATA         *obj;
    OBJ_DATA               *temp;

    argument = one_argument( argument, arg );

    /*
     * Temporarily disable this command. 
     */
/*    return;*/

    if ( arg[0] == '\0' ) {
        send_to_char( "Delete which object?\r\n", ch );
        return;
    }

    /*
     * Find the object. 
     */
    if ( !( obj = get_obj_index( atoi( arg ) ) ) ) {
        if ( !( temp = get_obj_here( ch, arg ) ) ) {
            send_to_char( "No such object.\r\n", ch );
            return;
        }
        obj = temp->pIndexData;
    }

    /*
     * Does the player have the right to delete this room? 
     */
    if ( get_trust( ch ) < sysdata.level_modify_proto
         && ( obj->vnum < ch->pcdata->o_range_lo || obj->vnum > ch->pcdata->o_range_hi ) ) {
        send_to_char( "That object is not in your assigned range.\r\n", ch );
        return;
    }

    /*
     * Ok, we've determined that the room exists, it is empty and the 
     * player has the authority to delete it, so let's dump the thing. 
     * The function to do it is in db.c so it can access the top-room 
     * variable. 
     */
    delete_obj( obj );

    send_to_char( "Object deleted.\r\n", ch );
    return;
}

void do_mdelete( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MOB_INDEX_DATA         *mob;
    CHAR_DATA              *temp;

    argument = one_argument( argument, arg );

    /*
     * Temporarily disable this command. 
     */
/*    return;*/

    if ( arg[0] == '\0' ) {
        send_to_char( "Delete which mob?\r\n", ch );
        return;
    }

    /*
     * Find the mob. 
     */
    if ( !( mob = get_mob_index( atoi( arg ) ) ) ) {
        if ( !( temp = get_char_room( ch, arg ) ) || !IS_NPC( temp ) ) {
            send_to_char( "No such mob.\r\n", ch );
            return;
        }
        mob = temp->pIndexData;
    }

    /*
     * Does the player have the right to delete this room? 
     */
    if ( get_trust( ch ) < sysdata.level_modify_proto
         && ( mob->vnum < ch->pcdata->m_range_lo || mob->vnum > ch->pcdata->m_range_hi ) ) {
        send_to_char( "That mob is not in your assigned range.\r\n", ch );
        return;
    }

    /*
     * Ok, we've determined that the mob exists and the player has the
     * authority to delete it, so let's dump the thing.
     * The function to do it is in db.c so it can access the top_mob_index
     * variable. 
     */
    delete_mob( mob );

    send_to_char( "Mob deleted.\r\n", ch );
    return;
}

/*  
 *  Mobile and Object Program Copying 
 *  Last modified Feb. 24 1999
 *  Mystaric
 */

void mpcopy( MPROG_DATA * source, MPROG_DATA * destination )
{
    destination->type = source->type;
    destination->triggered = source->triggered;
    destination->resetdelay = source->resetdelay;
    destination->arglist = STRALLOC( source->arglist );
    destination->comlist = STRALLOC( source->comlist );
    destination->next = NULL;
}

void do_opcopy( CHAR_DATA *ch, char *argument )
{
    char                    sobj[MIL];
    char                    prog[MIL];
    char                    num[MIL];
    char                    dobj[MIL];
    OBJ_DATA               *source = NULL,
        *destination = NULL;
    MPROG_DATA             *source_oprog = NULL,
        *dest_oprog = NULL,
        *source_oprg = NULL,
        *dest_oprg = NULL;
    int                     value = -1,
        optype = -1,
        cnt = 0;
    bool                    COPY = FALSE;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't opcopy\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    smash_tilde( argument );
    argument = one_argument( argument, sobj );
    argument = one_argument( argument, prog );

    if ( sobj[0] == '\0' || prog[0] == '\0' ) {
        send_to_char( "Syntax: opcopy <source object> <program> [number] <destination object>\r\n",
                      ch );
        send_to_char( "        opcopy <source object> all <destination object>\r\n", ch );
        send_to_char( "        opcopy <source object> all <destination object> <program>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Program being one of:\r\n", ch );
        send_to_char( "  act speech rand wear remove trash zap get\r\n", ch );
        send_to_char( "  drop damage repair greet exa use\r\n", ch );
        send_to_char( "  pull push (for levers,pullchains,buttons)\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Object should be in your inventory to edit.\r\n", ch );
        return;
    }

    if ( !strcmp( prog, "all" ) ) {
        argument = one_argument( argument, dobj );
        argument = one_argument( argument, prog );
        optype = get_mpflag( prog );
        COPY = TRUE;
    }
    else {
        argument = one_argument( argument, num );
        argument = one_argument( argument, dobj );
        value = atoi( num );
    }

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        if ( ( source = get_obj_carry( ch, sobj ) ) == NULL ) {
            send_to_char( "You aren't carrying source object.\r\n", ch );
            return;
        }

        if ( ( destination = get_obj_carry( ch, dobj ) ) == NULL ) {
            send_to_char( "You aren't carrying destination object.\r\n", ch );
            return;
        }
    }
    else {
        if ( ( source = get_obj_world( ch, sobj ) ) == NULL ) {
            send_to_char( "Can't find source object in all the realms.\r\n", ch );
            return;
        }

        if ( ( destination = get_obj_world( ch, dobj ) ) == NULL ) {
            send_to_char( "Can't find destination object in all the realms.\r\n", ch );
            return;
        }
    }

    if ( source == destination ) {
        send_to_char( "Source and destination objects cannot be the same\r\n", ch );
        return;
    }

    if ( !can_omodify( ch, destination ) ) {
        send_to_char( "You cannot modify destination object.\r\n", ch );
        return;
    }

    if ( !IS_OBJ_STAT( destination, ITEM_PROTOTYPE ) ) {
        send_to_char( "Destination object must have prototype flag.\r\n", ch );
        return;
    }

    set_char_color( AT_PLAIN, ch );

    source_oprog = source->pIndexData->mudprogs;
    dest_oprog = destination->pIndexData->mudprogs;

    set_char_color( AT_GREEN, ch );

    if ( !source_oprog ) {
        send_to_char( "Source object has no mob programs.\r\n", ch );
        return;
    }

    if ( COPY ) {
        for ( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next ) {
            if ( optype == source_oprg->type || optype == -1 ) {
                if ( dest_oprog != NULL )
                    for ( ; dest_oprog->next; dest_oprog = dest_oprog->next );
                CREATE( dest_oprg, MPROG_DATA, 1 );
                if ( dest_oprog )
                    dest_oprog->next = dest_oprg;
                else {
                    destination->pIndexData->mudprogs = dest_oprg;
                    dest_oprog = dest_oprg;
                }
                mpcopy( source_oprg, dest_oprg );
                xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
                cnt++;
            }
        }

        if ( cnt == 0 ) {
            ch_printf( ch, "No such program in source object\r\n" );
            return;
        }
        ch_printf( ch, "%d programs successfully copied from %s to %s.\r\n", cnt, sobj, dobj );
        return;
    }

    if ( value < 1 ) {
        send_to_char( "No such program in source object.\r\n", ch );
        return;
    }

    optype = get_mpflag( prog );

    for ( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next ) {
        if ( ++cnt == value && source_oprg->type == optype ) {
            if ( dest_oprog != NULL )
                for ( ; dest_oprog->next; dest_oprog = dest_oprog->next );
            CREATE( dest_oprg, MPROG_DATA, 1 );
            if ( dest_oprog )
                dest_oprog->next = dest_oprg;
            else
                destination->pIndexData->mudprogs = dest_oprg;
            mpcopy( source_oprg, dest_oprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
            ch_printf( ch, "%s program %d from %s successfully copied to %s.\r\n", prog, value,
                       sobj, dobj );
            return;
        }
    }
    if ( !source_oprg ) {
        send_to_char( "No such program in source object.\r\n", ch );
        return;
    }
    do_opcopy( ch, ( char * ) "" );
}

void do_mpcopy( CHAR_DATA *ch, char *argument )
{
    char                    smob[MIL];
    char                    prog[MIL];
    char                    num[MIL];
    char                    dmob[MIL];
    CHAR_DATA              *source = NULL,
        *destination = NULL;
    MPROG_DATA             *source_mprog = NULL,
        *dest_mprog = NULL,
        *source_mprg = NULL,
        *dest_mprg = NULL;
    int                     value = -1,
        mptype = -1,
        cnt = 0;
    bool                    COPY = FALSE;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't opcop\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor\r\n", ch );
        return;
    }

    smash_tilde( argument );
    argument = one_argument( argument, smob );
    argument = one_argument( argument, prog );

    if ( smob[0] == '\0' || prog[0] == '\0' ) {
        send_to_char( "Syntax: mpcopy <source mobile> <program> [number] <destination mobile>\r\n",
                      ch );
        send_to_char( "        mpcopy <source mobile> all <destination mobile>\r\n", ch );
        send_to_char( "        mpcopy <source mobile> all <destination mobile> <program>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Program being one of:\r\n", ch );
        send_to_char( "  act speech rand fight hitprcnt greet allgreet\r\n", ch );
        send_to_char( "  entry give bribe death time hour script syntax\r\n", ch );
        return;
    }

    if ( !strcmp( prog, "all" ) ) {
        argument = one_argument( argument, dmob );
        argument = one_argument( argument, prog );
        mptype = get_mpflag( prog );
        COPY = TRUE;
    }
    else {
        argument = one_argument( argument, num );
        argument = one_argument( argument, dmob );
        value = atoi( num );
    }

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        if ( ( source = get_char_room( ch, smob ) ) == NULL ) {
            send_to_char( "Source mobile is not present.\r\n", ch );
            return;
        }

        if ( ( destination = get_char_room( ch, dmob ) ) == NULL ) {
            send_to_char( "Destination mobile is not present.\r\n", ch );
            return;
        }
    }
    else {
        if ( ( source = get_char_world( ch, smob ) ) == NULL ) {
            send_to_char( "Can't find source mobile\r\n", ch );
            return;
        }

        if ( ( destination = get_char_world( ch, dmob ) ) == NULL ) {
            send_to_char( "Can't find destination mobile\r\n", ch );
            return;
        }
    }
    if ( source == destination ) {
        send_to_char( "Source and destination mobiles cannot be the same\r\n", ch );
        return;
    }

    if ( get_trust( ch ) < source->level || !IS_NPC( source )
         || get_trust( ch ) < destination->level || !IS_NPC( destination ) ) {
        send_to_char( "You can't do that!\r\n", ch );
        return;
    }

    if ( !can_mmodify( ch, destination ) ) {
        send_to_char( "You cannot modify destination mobile.\r\n", ch );
        return;
    }

    if ( !xIS_SET( destination->act, ACT_PROTOTYPE ) ) {
        send_to_char( "Destination mobile must have a prototype flag to mpcopy.\r\n", ch );
        return;
    }

    source_mprog = source->pIndexData->mudprogs;
    dest_mprog = destination->pIndexData->mudprogs;

    set_char_color( AT_GREEN, ch );

    if ( !source_mprog ) {
        send_to_char( "Source mobile has no mob programs.\r\n", ch );
        return;
    }

    if ( COPY ) {
        for ( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next ) {
            if ( mptype == source_mprg->type || mptype == -1 ) {
                if ( dest_mprog != NULL )
                    for ( ; dest_mprog->next; dest_mprog = dest_mprog->next );
                CREATE( dest_mprg, MPROG_DATA, 1 );

                if ( dest_mprog )
                    dest_mprog->next = dest_mprg;
                else {
                    destination->pIndexData->mudprogs = dest_mprg;
                    dest_mprog = dest_mprg;
                }
                mpcopy( source_mprg, dest_mprg );
                xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
                cnt++;
            }
        }

        if ( cnt == 0 ) {
            ch_printf( ch, "No such program in source mobile\r\n" );
            return;
        }
        ch_printf( ch, "%d programs successfully copied from %s to %s.\r\n", cnt, smob, dmob );
        return;
    }

    if ( value < 1 ) {
        send_to_char( "No such program in source mobile.\r\n", ch );
        return;
    }

    mptype = get_mpflag( prog );

    for ( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next ) {
        if ( ++cnt == value && source_mprg->type == mptype ) {
            if ( dest_mprog != NULL )
                for ( ; dest_mprog->next; dest_mprog = dest_mprog->next );
            CREATE( dest_mprg, MPROG_DATA, 1 );
            if ( dest_mprog )
                dest_mprog->next = dest_mprg;
            else
                destination->pIndexData->mudprogs = dest_mprg;
            mpcopy( source_mprg, dest_mprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
            ch_printf( ch, "%s program %d from %s successfully copied to %s.\r\n", prog, value,
                       smob, dmob );
            return;
        }
    }

    if ( !source_mprg ) {
        send_to_char( "No such program in source mobile.\r\n", ch );
        return;
    }
    do_mpcopy( ch, ( char * ) "" );
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelCreate( relation_type tp, void *actor, void *subject )
{
    REL_DATA               *tmp;

    if ( tp < relMSET_ON || tp > relOSET_ON ) {
        bug( "RelCreate: invalid type (%d)", tp );
        return;
    }
    if ( !actor ) {
        bug( "RelCreate: NULL actor" );
        return;
    }
    if ( !subject ) {
        bug( "RelCreate: NULL subject" );
        return;
    }
    for ( tmp = first_relation; tmp; tmp = tmp->next )
        if ( tmp->Type == tp && tmp->Actor == actor && tmp->Subject == subject ) {
            bug( "RelCreate: duplicated relation" );
            return;
        }
    CREATE( tmp, REL_DATA, 1 );
    tmp->Type = tp;
    tmp->Actor = actor;
    tmp->Subject = subject;
    LINK( tmp, first_relation, last_relation, next, prev );
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelDestroy( relation_type tp, void *actor, void *subject )
{
    REL_DATA               *rq;

    if ( tp < relMSET_ON || tp > relOSET_ON ) {
        bug( "RelDestroy: invalid type (%d)", tp );
        return;
    }
    if ( !actor ) {
        bug( "RelDestroy: NULL actor" );
        return;
    }
    if ( !subject ) {
        bug( "RelDestroy: NULL subject" );
        return;
    }
    for ( rq = first_relation; rq; rq = rq->next )
        if ( rq->Type == tp && rq->Actor == actor && rq->Subject == subject ) {
            UNLINK( rq, first_relation, last_relation, next, prev );
            /*
             * Dispose will also set to NULL the passed parameter 
             */
            DISPOSE( rq );
            break;
        }
}

/* Shogar's code to hunt for exits/entrances to/from a zone, very nice */
/* Display improvements and overland support by Samson of Alsherok */
void do_aexit( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room;
    int                     i,
                            vnum;
    AREA_DATA              *tarea;
    AREA_DATA              *otherarea;
    EXIT_DATA              *pexit;
    int                     lrange;
    int                     trange;
    bool                    found = FALSE;

    if ( argument[0] == '\0' )
        tarea = ch->in_room->area;
    else {
        for ( tarea = first_area; tarea; tarea = tarea->next )
            if ( !str_cmp( tarea->filename, argument ) ) {
                found = TRUE;
                break;
            }

        if ( !found ) {
            for ( tarea = first_build; tarea; tarea = tarea->next )
                if ( !str_cmp( tarea->filename, argument ) ) {
                    found = TRUE;
                    break;
                }
        }

        if ( !found ) {
            send_to_char( "Area not found. Check 'zones' for the filename.\r\n", ch );
            return;
        }
    }

    trange = tarea->hi_r_vnum;
    lrange = tarea->low_r_vnum;

    for ( vnum = lrange; vnum <= trange; vnum++ ) {
        if ( ( room = get_room_index( vnum ) ) == NULL )
            continue;

        if ( IS_SET( room->room_flags, ROOM_TELEPORT )
             && ( room->tele_vnum < lrange || room->tele_vnum > trange ) ) {
            pager_printf( ch, "From: %-20.20s Room: %5d To: Room: %5d (Teleport)\r\n",
                          tarea->filename, vnum, room->tele_vnum );
        }

        for ( i = 0; i < MAX_DIR + 1; i++ ) {
            if ( ( pexit = get_exit( room, i ) ) == NULL )
                continue;
            if ( pexit->to_room->area != tarea ) {
                pager_printf( ch, "To: %-20.20s Room: %5d From: %-20.20s Room: %5d (%s)\r\n",
                              pexit->to_room->area->filename, pexit->vnum, tarea->filename, vnum,
                              dir_name[i] );
            }
        }
    }

    for ( otherarea = first_area; otherarea; otherarea = otherarea->next ) {
        if ( tarea == otherarea )
            continue;
        trange = otherarea->hi_r_vnum;
        lrange = otherarea->low_r_vnum;
        for ( vnum = lrange; vnum <= trange; vnum++ ) {
            if ( ( room = get_room_index( vnum ) ) == NULL )
                continue;

            if ( IS_SET( room->room_flags, ROOM_TELEPORT ) ) {
                if ( room->tele_vnum >= tarea->low_r_vnum && room->tele_vnum <= tarea->hi_r_vnum )
                    pager_printf( ch,
                                  "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (Teleport)\r\n",
                                  otherarea->filename, vnum, tarea->filename, room->tele_vnum );
            }

            for ( i = 0; i < MAX_DIR + 1; i++ ) {
                if ( ( pexit = get_exit( room, i ) ) == NULL )
                    continue;

                if ( pexit->to_room->area == tarea ) {
                    pager_printf( ch, "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (%s)\r\n",
                                  otherarea->filename, vnum, pexit->to_room->area->filename,
                                  pexit->vnum, dir_name[i] );
                }
            }
        }
    }

    return;
}

void do_makerooms( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *location;
    AREA_DATA              *pArea;
    int                     vnum,
                            x;
    int                     room_count;

    pArea = ch->pcdata->area;

    if ( !pArea ) {
        send_to_char( "You must have an area assigned to do this.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Create a block of rooms.\r\n", ch );
        send_to_char( "Usage: makerooms <# of rooms>\r\n", ch );
        return;
    }
    x = atoi( argument );

    ch_printf( ch, "Attempting to create a block of %d rooms.\r\n", x );

    if ( x > 1000 ) {
        ch_printf( ch, "The maximum number of rooms this mud can create at once is 1000.\r\n" );
        return;
    }
    room_count = 0;

    if ( !IS_SET( pArea->status, AREA_LOADED ) ) {
        send_to_char( "Your vnum range isn't loaded!\r\n", ch );
        return;
    }

    if ( pArea->low_r_vnum + x > pArea->hi_r_vnum ) {
        send_to_char( "You don't even have that many rooms assigned to you.\r\n", ch );
        return;
    }

    for ( vnum = pArea->low_r_vnum; vnum <= pArea->hi_r_vnum; vnum++ ) {
        if ( get_room_index( vnum ) == NULL )
            room_count++;

        if ( room_count >= x )
            break;
    }

    if ( room_count < x ) {
        send_to_char( "There aren't enough free rooms in your assigned range!\r\n", ch );
        return;
    }

    send_to_char( "Creating the rooms...\r\n", ch );
    room_count = 0;
    vnum = pArea->low_r_vnum;

    while ( room_count < x ) {
        if ( get_room_index( vnum ) == NULL ) {
            room_count++;

            location = make_room( vnum, ch->pcdata->area );
            if ( !location ) {
                bug( "do_makerooms: make_room failed" );
                return;
            }
            location->area = ch->pcdata->area;
        }
        vnum++;
    }

    ch_printf( ch, "%d rooms created.\r\n", room_count );
    return;
}

/* Consolidated *assign function. 
 * Assigns room/obj/mob ranges and initializes new zone - Samson 2-12-99 
 */
/* Bugfix: Vnum range would not be saved properly without placeholders at both ends - Samson 
1-6-00 */
void do_vassign( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    int                     lo,
                            hi;
    CHAR_DATA              *victim,
                           *mob;
    ROOM_INDEX_DATA        *room;
    MOB_INDEX_DATA         *pMobIndex;
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;
    AREA_DATA              *tarea;
    char                    filename[256];

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    lo = atoi( arg2 );
    hi = atoi( arg3 );

    if ( arg1[0] == '\0' || lo < 0 || hi < 0 ) {
        send_to_char( "Syntax: vassign <who> <low> <high>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They don't seem to be around.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "They wouldn't know what to do with a vnum range.\r\n", ch );
        return;
    }
    if ( victim->pcdata->area && lo != 0 ) {
        send_to_char( "You cannot assign them a range, they already have one!\r\n", ch );
        return;
    }
    if ( lo > hi ) {
        send_to_char( "Unacceptable vnum range.\r\n", ch );
        return;
    }
    if ( lo == 0 )
        hi = 0;
    if ( !vnums_are_free( "all", lo, hi ) ) {
        send_to_char( "Those vnums are already being used in another area.\r\n", ch );
        return;
    }
    victim->pcdata->r_range_lo = lo;
    victim->pcdata->r_range_hi = hi;
    victim->pcdata->o_range_lo = lo;
    victim->pcdata->o_range_hi = hi;
    victim->pcdata->m_range_lo = lo;
    victim->pcdata->m_range_hi = hi;
    assign_area( victim, FALSE );
    send_to_char( "Done.\r\n", ch );
    ch_printf( victim, "%s has assigned you the vnum range %d - %d.\r\n", ch->name, lo, hi );
    assign_area( victim, FALSE );                      /* Put back by Thoric on 02/07/96 */

    if ( !victim->pcdata->area ) {
        bug( "vassign: assign_area failed" );
        return;
    }

    tarea = victim->pcdata->area;

    if ( lo == 0 ) {                                   /* Scryn 8/12/95 */
        REMOVE_BIT( tarea->status, AREA_LOADED );
        SET_BIT( tarea->status, AREA_DELETED );
    }
    else {
        SET_BIT( tarea->status, AREA_LOADED );
        REMOVE_BIT( tarea->status, AREA_DELETED );
    }

    /*
     * Initialize first and last rooms in range 
     */
    if ( !( room = make_room( lo, tarea ) ) ) {
        bug( "do_vassign: make_room failed to initialize first room." );
        return;
    }

    if ( !( room = make_room( hi, tarea ) ) ) {
        bug( "do_vassign: make_room failed to initialize last room." );
        return;
    }

    /*
     * Initialize first mob in range 
     */
    pMobIndex = make_mobile( lo, 0, ( char * ) "first mob" );
    if ( !pMobIndex ) {
        log_string( "do_vassign: make_mobile failed to initialize first mob." );
        return;
    }
    mob = create_mobile( pMobIndex );
    char_to_room( mob, room );

    /*
     * Initialize last mob in range 
     */
    pMobIndex = make_mobile( hi, 0, ( char * ) "last mob" );
    if ( !pMobIndex ) {
        log_string( "do_vassign: make_mobile failed to initialize last mob." );
        return;
    }
    mob = create_mobile( pMobIndex );
    char_to_room( mob, room );

    /*
     * Initialize first obj in range 
     */
    pObjIndex = make_object( lo, 0, ( char * ) "first obj" );
    if ( !pObjIndex ) {
        log_string( "do_vassign: make_object failed to initialize first obj." );
        return;
    }
    obj = create_object( pObjIndex, 0 );
    obj_to_room( obj, room );

    /*
     * Initialize last obj in range 
     */
    pObjIndex = make_object( hi, 0, ( char * ) "last obj" );
    if ( !pObjIndex ) {
        log_string( "do_vassign: make_object failed to initialize last obj." );
        return;
    }
    obj = create_object( pObjIndex, 0 );
    obj_to_room( obj, room );

    /*
     * Save character and newly created zone 
     */
    save_char_obj( victim );

    if ( !IS_SET( tarea->status, AREA_DELETED ) ) {
        snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
        fold_area( tarea, filename, FALSE );
    }

    set_char_color( AT_IMMORT, ch );
    ch_printf( ch, "Vnum range set for %s and initialized.\r\n", victim->name );
}

int generate_hp( int level, int num, int size, int plus )
{
    int                     max_hit = 0;

    if ( !num )
        max_hit = level * 8 + number_range( level * level / 4, level * level );
    else
        max_hit = num * number_range( 1, size ) + plus;
    return max_hit;
}

/* Volk - command to change all sizes on equipment in an area */
void do_aobjsize( CHAR_DATA *ch, char *argument )
{
    char                   *arg1;
    short                   size;
    AREA_DATA              *area;
    bool                    yes = FALSE;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg1 );

    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Syntax: aobjsize (filename.are) (size)\r\n", ch );
        send_to_char
            ( "ie 'aobjsize fireant.are 3' would resize all objects in Fire Ant area to 3.\r\n",
              ch );
        return;
    }

/* Two arguments - let's check whether we've picked a valid area and size. */
    for ( area = first_area; area; area = area->next ) {
        if ( !str_cmp( area->filename, arg1 ) ) {
            yes = TRUE;
            break;
        }
    }

    if ( !yes || !is_number( argument ) ) {            /* didn't find an area */
        send_to_char( "Syntax: aobjsize (filename.are) (size)\r\n", ch );
        send_to_char
            ( "ie 'aobjsize fireant.are 3' would resize all objects in Fire Ant area to 3.\r\n",
              ch );
        return;
    }

    size = atoi( argument );

    if ( size > 7 || size < 0 ) {
        send_to_char( "Size must be between 0 and 7. See HELP OBJECTSIZES.\r\n", ch );
        return;
    }

    int                     vnum;
    OBJ_INDEX_DATA         *obj;
    int                     count = 0;

    for ( vnum = area->low_o_vnum; vnum <= area->hi_o_vnum; vnum++ ) {
//    if( (obj = (get_obj_index(vnum)) == NULL))
        obj = get_obj_index( vnum );
        if ( obj == NULL )
            continue;
        count++;
        obj->size = size;
    }

    ch_printf( ch, "Done, changed the sizes on %d objects.\r\n", count );
    send_to_char( "Please FOLDAREA (filename) to save changes.\r\n", ch );

    return;
}
