#include "h/mud.h"
#include "h/files.h"
#include "h/languages.h"
#include "h/key.h"
#ifndef WIN32
#include <unistd.h>
#endif

int                     get_risflag( char *flag );
int                     get_langflag( char *flag );
int                     get_langnum( char *flag );
int                     get_defenseflag( char *flag );
int                     get_attackflag( char *flag );

extern bool             load_race_file( const char *fname );
void                    write_race_file( int ra );
int                     MAX_PC_RACE;

RACE_TYPE              *race_table[MAX_RACE];

const char             *const npc_race[MAX_NPC_RACE] = {
    "human", "elf", "dwarf", "halfling", "pixie", "ogre",   // 5 with human zero
    "orc", "troll", "shade", "goblin", "drow", "gnome", // 11
    "centaur", "dragon", "vampire", "celestial", "demon", "mindflayer", // 17

/* Above this line should be all player races */
    "ant", "ape", "baboon", "bat", "bear",             // 22
    "bee", "beetle", "boar", "bugbear", "cat", "dog",  // 28
    "plant", "ferret", "fly", "gargoyle", "gelatin", "ghoul",   // 34
    "gnoll", "r29", "golem", "gorgon", "harpy",        // 39
    "hobgoblin", "kobold", "locust", "lycanthrope", "minotaur", // 44
    "mold", "mule", "neanderthal", "ooze", "r43", "rat",    // 50
    "rustmonster", "shadow", "shapeshifter", "shrew", "shrieker", "skeleton",   // 56
    "slime", "snake", "spider", "stirge", "thoul", "troglodyte",    // 62
    "undead", "wight", "wolf", "worm", "zombie", "bovine",  // 68
    "canine", "feline", "porcine", "mammal", "rodent", "avis",  // 74
    "reptile", "amphibian", "fish", "crustacean", "insect", "spirit",   // 80
    "magical", "horse", "animal", "humanoid", "monster", "god", // 86
    "avian", "badger", "beaver", "wolverine", "lion"   // 92
};

char                   *const raceflags[] = {
    ( char * ) "advanced"
};

int                     get_raceflag( char *flag );

void free_all_races( void )
{
    int                     hash,
                            loopa;

    for ( hash = 0; hash < MAX_RACE; hash++ ) {
        for ( loopa = 0; loopa < MAX_WHERE_NAME; loopa++ )
            if ( race_table[hash]->where_name[loopa] )
                STRFREE( race_table[hash]->where_name[loopa] );
        DISPOSE( race_table[hash] );
    }
}

/*
 * Create an instance of a new race.   -Shaddai
 */
bool create_new_race( int race, char *argument )
{
    int                     i = 0;

    if ( !VLD_STR( argument ) )
        return FALSE;
    if ( race >= MAX_RACE || race_table[race] == NULL )
        return FALSE;
    for ( i = 0; i <= MAX_WHERE_NAME; i++ )
        race_table[race]->where_name[i] = STRALLOC( where_name[i] );
    argument[0] = UPPER( argument[0] );
    snprintf( race_table[race]->race_name, 16, "%-.16s", argument );
    race_table[race]->sdbonus = 0;
    race_table[race]->base_str = 0;
    race_table[race]->base_dex = 0;
    race_table[race]->base_wis = 0;
    race_table[race]->base_int = 0;
    race_table[race]->base_con = 0;
    race_table[race]->base_cha = 0;
    race_table[race]->base_lck = 0;
    race_table[race]->hit = 0;
    race_table[race]->mana = 0;
    xCLEAR_BITS( race_table[race]->affected );
    race_table[race]->resist = 0;
    race_table[race]->suscept = 0;
    race_table[race]->language = 0;
    race_table[race]->alignment = 0;
    race_table[race]->minalign = 0;
    race_table[race]->maxalign = 0;
    race_table[race]->ac_plus = 0;
    race_table[race]->exp_multiplier = 0;
    xCLEAR_BITS( race_table[race]->attacks );
    xCLEAR_BITS( race_table[race]->defenses );
    race_table[race]->height = 0;
    race_table[race]->weight = 0;
    race_table[race]->hunger_mod = 0;
    race_table[race]->thirst_mod = 0;
    race_table[race]->mana_regen = 0;
    race_table[race]->hp_regen = 0;
    race_table[race]->race_recall = 0;
    return TRUE;
}

/*
 * Load in all the race files.
 */
void load_races(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    racelist[256];
    int                     i;

    MAX_PC_RACE = 0;
    /*
     * Pre-init the race_table with blank races
     */
    for ( i = 0; i < MAX_RACE; i++ )
        race_table[i] = NULL;

    snprintf( racelist, 256, "%s%s", RACEDIR, RACE_LIST );
    if ( ( fpList = FileOpen( racelist, "r" ) ) == NULL ) {
        perror( racelist );
        exit( 1 );
    }

    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        if ( filename[0] == '$' )
            break;

        if ( !load_race_file( filename ) ) {
            bug( "Cannot load race file: %s", filename );
        }
        else
            MAX_PC_RACE++;
    }
    for ( i = 0; i < MAX_RACE; i++ ) {
        if ( race_table[i] == NULL ) {
            CREATE( race_table[i], struct race_type, 1 );

            snprintf( race_table[i]->race_name, 16, "%s", "unused" );
        }
    }
    FileClose( fpList );
    return;
}

void write_race_file( int ra )
{
    FILE                   *fpout;
    char                    filename[MIL];
    struct race_type       *race = race_table[ra];
    int                     i;
    int                     x,
                            y;

    if ( !race->race_name ) {
        bug( "Race %d has null name, not writing .race file.", ra );
        return;
    }

    snprintf( filename, MIL, "%s%s.race", RACEDIR, race->race_name );
    if ( ( fpout = FileOpen( filename, "w+" ) ) == NULL ) {
        bug( "Cannot open: %s for writing", filename );
        return;
    }

    if ( VLD_STR( race->race_name ) )
        fprintf( fpout, "Name        %s~\n", race->race_name );
    fprintf( fpout, "Race        %d\n", ra );
    fprintf( fpout, "SDbonus     %d\n", race->sdbonus );
    fprintf( fpout, "Base_Str    %d\n", race->base_str );
    fprintf( fpout, "Base_Dex    %d\n", race->base_dex );
    fprintf( fpout, "Base_Wis    %d\n", race->base_wis );
    fprintf( fpout, "Base_Int    %d\n", race->base_int );
    fprintf( fpout, "Base_Con    %d\n", race->base_con );
    fprintf( fpout, "Base_Cha    %d\n", race->base_cha );
    fprintf( fpout, "Base_Lck    %d\n", race->base_lck );
    fprintf( fpout, "Hit         %d\n", race->hit );
    fprintf( fpout, "Mana        %d\n", race->mana );
    fprintf( fpout, "Affected    %s\n", print_bitvector( &race->affected ) );
    fprintf( fpout, "Resist      %d\n", race->resist );
    fprintf( fpout, "Suscept     %d\n", race->suscept );
    fprintf( fpout, "Language    %d\n", race->language );
    fprintf( fpout, "Align       %d\n", race->alignment );
    fprintf( fpout, "Min_Align   %d\n", race->minalign );
    fprintf( fpout, "Max_Align   %d\n", race->maxalign );
    fprintf( fpout, "AC_Plus     %d\n", race->ac_plus );
    fprintf( fpout, "Exp_Mult    %d\n", race->exp_multiplier );
    if ( !xIS_EMPTY( race->attacks ) )
        fprintf( fpout, "Attacks     %s\n", print_bitvector( &race->attacks ) );
    if ( !xIS_EMPTY( race->defenses ) )
        fprintf( fpout, "Defenses    %s\n", print_bitvector( &race->defenses ) );
    fprintf( fpout, "Height      %d\n", race->height );
    fprintf( fpout, "Weight      %d\n", race->weight );
    fprintf( fpout, "Flags	      %s\n", print_bitvector( &race->flags ) );
    fprintf( fpout, "Hunger_Mod  %d\n", race->hunger_mod );
    fprintf( fpout, "Thirst_mod  %d\n", race->thirst_mod );
    fprintf( fpout, "Mana_Regen  %d\n", race->mana_regen );
    fprintf( fpout, "HP_Regen    %d\n", race->hp_regen );
    fprintf( fpout, "Race_Recall %d\n", race->race_recall );
    for ( i = 0; i < MAX_WHERE_NAME; i++ )
        fprintf( fpout, "WhereName  %s~\n", race->where_name[i] );

    for ( x = 0; x < top_sn; x++ ) {
        if ( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
            break;
        if ( ( y = skill_table[x]->race_level[ra] ) < LEVEL_IMMORTAL )
            fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y,
                     skill_table[x]->race_adept[ra] );
    }
    fprintf( fpout, "End\n" );
    FileClose( fpout );
}

bool load_race_file( const char *fname )
{
    char                    buf[MSL];
    const char             *word;
    char                   *race_name = NULL;
    bool                    fMatch;
    struct race_type       *race;
    int                     ra = -1;
    FILE                   *fp;
    int                     i,
                            wear = 0;

    snprintf( buf, MSL, "%s%s", RACEDIR, fname );
//  snprintf(buf, MSL, "%s", fname);
    if ( ( fp = FileOpen( buf, "r" ) ) == NULL ) {
        perror( buf );
        return FALSE;
    }

    CREATE( race, struct race_type, 1 );
    for ( i = 0; i < MAX_WHERE_NAME; i++ )
        race->where_name[i] = STRALLOC( where_name[i] );

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "Align", race->alignment, fread_number( fp ) );
                KEY( "AC_Plus", race->ac_plus, fread_number( fp ) );
                KEY( "Affected", race->affected, fread_bitvector( fp ) );
                KEY( "Attacks", race->attacks, fread_bitvector( fp ) );
                break;

            case 'B':
                KEY( "Base_Dex", race->base_dex, fread_number( fp ) );
                KEY( "Base_Int", race->base_int, fread_number( fp ) );
                KEY( "Base_Lck", race->base_lck, fread_number( fp ) );
                KEY( "Base_Con", race->base_con, fread_number( fp ) );
                KEY( "Base_Cha", race->base_cha, fread_number( fp ) );
                KEY( "Base_Wis", race->base_wis, fread_number( fp ) );
                KEY( "Base_Str", race->base_str, fread_number( fp ) );
                break;

            case 'D':
                KEY( "Defenses", race->defenses, fread_bitvector( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    FileClose( fp );
                    if ( ra < 0 || ra >= MAX_RACE ) {
                        bug( "Load_race_file: Race (%s) bad/not found (%d)",
                             race->race_name ? race->
                             race_name : race_name ? race_name : "name not found", ra );
                        if ( race_name )
                            STRFREE( race_name );
                        DISPOSE( race );
                        return FALSE;
                    }
                    race_table[ra] = race;
                    if ( race_name )
                        STRFREE( race_name );
                    return TRUE;
                }
                KEY( "Exp_Mult", race->exp_multiplier, fread_number( fp ) );
                break;

            case 'F':
                KEY( "Flags", race->flags, fread_bitvector( fp ) );
                break;

            case 'H':
                KEY( "Height", race->height, fread_number( fp ) );
                KEY( "Hit", race->hit, fread_number( fp ) );
                KEY( "HP_Regen", race->hp_regen, fread_number( fp ) );
                KEY( "Hunger_Mod", race->hunger_mod, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Language", race->language, fread_number( fp ) );
                break;

            case 'M':
                KEY( "Mana", race->mana, fread_number( fp ) );
                KEY( "Mana_Regen", race->mana_regen, fread_number( fp ) );
                KEY( "Min_Align", race->minalign, fread_number( fp ) );
                KEY( "Max_Align", race->maxalign, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", race_name, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Race", ra, fread_number( fp ) );
                KEY( "Race_Recall", race->race_recall, fread_number( fp ) );
                KEY( "Resist", race->resist, fread_number( fp ) );
                break;

            case 'S':
                KEY( "SDbonus", race->sdbonus, fread_number( fp ) );
                KEY( "Suscept", race->suscept, fread_number( fp ) );
                if ( !str_cmp( word, "Skill" ) ) {
                    int                     sn,
                                            lev,
                                            adp;

                    word = fread_word( fp );
                    lev = fread_number( fp );
                    adp = fread_number( fp );
                    sn = skill_lookup( word );
                    if ( ra < 0 || ra >= MAX_RACE )
                        bug( "load_race_file: Skill %s -- race bad/not found (%d)", word, ra );
                    else if ( !IS_VALID_SN( sn ) )
                        bug( "load_race_file: Skill %s unknown", word );
                    else {
                        skill_table[sn]->race_level[ra] = lev;
                        skill_table[sn]->race_adept[ra] = adp;
                    }
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'T':
                KEY( "Thirst_Mod", race->thirst_mod, fread_number( fp ) );
                break;

            case 'W':
                KEY( "Weight", race->weight, fread_number( fp ) );
                if ( !str_cmp( word, "WhereName" ) ) {
                    if ( ra < 0 || ra >= MAX_RACE ) {
                        bug( "load_race_file: Title -- race bad/not found (%d)", ra );
                        fread_flagstring( fp );
                        fread_flagstring( fp );
                    }
                    else if ( wear < MAX_WHERE_NAME ) {
                        if ( VLD_STR( race->where_name[wear] ) )
                            STRFREE( race->where_name[wear] );
                        race->where_name[wear] = fread_string( fp );
                        ++wear;
                    }
                    else
                        bug( "%s", "load_race_file: Too many where_names" );
                    fMatch = TRUE;
                    break;
                }
                break;
        }

        if ( race_name != NULL )
            snprintf( race->race_name, 16, "%-.16s", race_name );

        if ( !fMatch ) {
            bug( "load_race_file: no match: %s", word );
            fread_to_eol( fp );
        }
    }
    return FALSE;
}

void do_races( CHAR_DATA *ch, char *argument )
{
    int                     iRace = 0;
    int                     counter = 0;

    if ( !IS_IMMORTAL( ch ) )
        counter = 1;
    send_to_pager( "\r\n", ch );
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ ) {
        if ( !IS_IMMORTAL( ch ) && !VLD_STR( race_table[iRace]->race_name ) )
            continue;
        pager_printf( ch, "&g[&G%2d&g]&W %-10s", counter, race_table[iRace]->race_name );
        if ( IS_IMMORTAL( ch ) )
            pager_printf( ch, "%-1s &gMultiplier:&W %-5d &gHit: &W%-3d &gMana: &W%-3d", "",
                          race_table[iRace]->exp_multiplier, race_table[iRace]->hit,
                          race_table[iRace]->mana );
        pager_printf( ch, "\r\n" );
        counter++;
    }
    return;
}

void update_racelist( void )
{
    char                    racelist[256];
    int                     i;
    FILE                   *fpList;

    snprintf( racelist, 256, "%s%s", RACEDIR, RACE_LIST );
    if ( ( fpList = FileOpen( racelist, "w" ) ) == NULL ) {
        bug( "%s", "Error opening racelist." );
        return;
    }
    for ( i = 0; i < MAX_PC_RACE; i++ )
        fprintf( fpList, "%s.race\n", race_table[i]->race_name );
    fprintf( fpList, "%s", "$\n" );
    FileClose( fpList );
    return;
}

/* Modified by Samson to allow setting language by name - 8-6-98 */
void do_setrace( CHAR_DATA *ch, char *argument )
{
    struct race_type       *race;
    char                    buf[MSL],
                            arg1[MIL],
                            arg2[MIL],
                            arg3[MIL];
    int                     value,
                            v2,
                            ra;

    set_char_color( AT_PLAIN, ch );

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: setrace <race> <field> <value>\r\n", ch );
        send_to_char( "Syntax: setrace <race> create\r\n", ch );
        send_to_char( "Syntax: setrace <race> save\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( "  name strplus dexplus wisplus\r\n", ch );
        send_to_char( "  intplus conplus chaplus lckplus hit\r\n", ch );
        send_to_char( "  mana affected resist suscept language\r\n", ch );
        send_to_char( "  attack defense alignment acplus\r\n", ch );
        send_to_char( "  minalign maxalign height weight\r\n", ch );
        send_to_char( "  hungermod thirstmod expmultiplier\r\n", ch );
        send_to_char( "  saving_poison_death saving_wand\r\n", ch );
        send_to_char( "  saving_para_petri saving_breath\r\n", ch );
        send_to_char( "  saving_spell_staff\r\n", ch );
        send_to_char( "  mana_regen hp_regen sdbonus\r\n", ch );
        send_to_char( "  flags\r\n", ch );
        return;
    }
    if ( is_number( arg1 ) && ( ra = atoi( arg1 ) ) >= 0 && ra < MAX_RACE )
        race = race_table[ra];
    else {
        race = NULL;
        for ( ra = 0; ra < MAX_RACE && race_table[ra]; ra++ ) {
            if ( !race_table[ra]->race_name )
                continue;

            if ( !str_cmp( race_table[ra]->race_name, arg1 ) ) {
                race = race_table[ra];
                break;
            }
        }
    }

    if ( !str_cmp( arg2, "create" ) && race ) {
        send_to_char( "That race already exists!\r\n", ch );
        return;
    }
    else if ( !race && str_cmp( arg2, "create" ) ) {
        send_to_char( "No such race.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "create" ) ) {
        if ( MAX_PC_RACE >= MAX_RACE ) {
            send_to_char( "You need to up MAX_RACE in mud.h and make clean.\r\n", ch );
            return;
        }
        if ( ( create_new_race( MAX_PC_RACE, arg1 ) ) == FALSE ) {
            send_to_char( "Couldn't create a new race.\r\n", ch );
            return;
        }
        MAX_PC_RACE++;
        update_racelist(  );
        ra = MAX_PC_RACE;
        write_race_file( MAX_PC_RACE );
        send_to_char( "New race created.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "save" ) ) {
        write_race_file( ra );
        send_to_char( "Saved.\r\n", ch );
        return;
    }

    if ( !VLD_STR( argument ) ) {
        send_to_char( "You must specify an argument.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        snprintf( buf, MSL, "%s%s.race", RACEDIR, race->race_name );
        snprintf( race->race_name, 16, "%-.16s", capitalize( argument ) );
        write_race_file( ra );
        unlink( buf );
        update_racelist(  );
        send_to_char( "Race name set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        value = get_raceflag( argument );

        if ( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\r\n", argument );
        else {
            xTOGGLE_BIT( race->flags, value );
            send_to_char( "Done.\r\n", ch );
        }

        return;
    }

    if ( !str_cmp( arg2, "sdbonus" ) )
        race->sdbonus = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "basestr" ) )
        race->base_str = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "basedex" ) )
        race->base_dex = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "basewis" ) )
        race->base_wis = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "baseint" ) )
        race->base_int = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "basecon" ) )
        race->base_con = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "basecha" ) )
        race->base_cha = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "baselck" ) )
        race->base_lck = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "hit" ) )
        race->hit = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "mana" ) )
        race->mana = ( short ) atoi( argument );
    else if ( !str_cmp( arg2, "affected" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setrace <race> affected <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_aflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( race->affected, value );
        }
    }
    else if ( !str_cmp( arg2, "resist" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setrace <race> resist <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( race->resist, 1 << value );
        }
    }
    else if ( !str_cmp( arg2, "suscept" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setrace <race> suscept <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                TOGGLE_BIT( race->suscept, 1 << value );
        }
    }
    else if ( !str_cmp( arg2, "language" ) ) {
        argument = one_argument( argument, arg3 );
        value = get_langflag( arg3 );
        if ( value == LANG_UNKNOWN ) {
            ch_printf( ch, "Unknown language: %s\r\n", arg3 );
            return;
        }
        else {
            if ( !( value &= VALID_LANGS ) )
                ch_printf( ch, "Player races may not speak %s.\r\n", arg3 );
        }
        v2 = get_langnum( arg3 );
        if ( v2 == -1 )
            ch_printf( ch, "Unknown language: %s\r\n", arg3 );
        else
            TOGGLE_BIT( race->language, 1 << v2 );
    }
    else if ( !str_cmp( arg2, "acplus" ) )
        race->ac_plus = atoi( argument );
    else if ( !str_cmp( arg2, "alignment" ) )
        race->alignment = atoi( argument );
    else if ( !str_cmp( arg2, "defense" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setrace <race> defense <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_defenseflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( race->defenses, value );
        }
    }
    else if ( !str_cmp( arg2, "attack" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setrace <race> attack <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg3 );
            value = get_attackflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
                xTOGGLE_BIT( race->attacks, value );
        }
    }
    else if ( !str_cmp( arg2, "minalign" ) )
        race->minalign = atoi( argument );
    else if ( !str_cmp( arg2, "maxalign" ) )
        race->maxalign = atoi( argument );
    else if ( !str_cmp( arg2, "height" ) )
        race->height = atoi( argument );
    else if ( !str_cmp( arg2, "weight" ) )
        race->weight = atoi( argument );
    else if ( !str_cmp( arg2, "thirstmod" ) )
        race->thirst_mod = atoi( argument );
    else if ( !str_cmp( arg2, "hungermod" ) )
        race->hunger_mod = atoi( argument );
    else if ( !str_cmp( arg2, "maxalign" ) )
        race->maxalign = atoi( argument );
    else if ( !str_cmp( arg2, "expmultiplier" ) )
        race->exp_multiplier = atoi( argument );
    else if ( !str_cmp( arg2, "saving_poison_death" ) )
        race->saving_poison_death = atoi( argument );
    else if ( !str_cmp( arg2, "saving_wand" ) )
        race->saving_wand = atoi( argument );
    else if ( !str_cmp( arg2, "saving_para_petri" ) )
        race->saving_para_petri = atoi( argument );
    else if ( !str_cmp( arg2, "saving_breath" ) )
        race->saving_breath = atoi( argument );
    else if ( !str_cmp( arg2, "saving_spell_staff" ) )
        race->saving_spell_staff = atoi( argument );
    else if ( !str_cmp( arg2, "mana_regen" ) )
        race->mana_regen = atoi( argument );
    else if ( !str_cmp( arg2, "hp_regen" ) )
        race->hp_regen = atoi( argument );
#ifdef NEW_RACE_STUFF
    else if ( !str_cmp( arg2, "carry_weight" ) )
        race->acplus = atoi( argument );
    else if ( !str_cmp( arg2, "carry_number" ) )
        race->acplus = atoi( argument );
#endif
    else {
        do_setrace( ch, ( char * ) "" );
        return;
    }
    write_race_file( ra );
    ch_printf( ch, "Updated %s on race %s[%d].\r\n", arg2,
               ( race_table[ra]
                 && VLD_STR( race_table[ra]->race_name ) ) ? race_table[ra]->
               race_name : "(Unknown)", ra );
    return;
}

void do_showrace( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    struct race_type       *race;
    int                     ra,
                            i;

    set_pager_color( AT_PLAIN, ch );
    argument = one_argument( argument, arg1 );
    if ( !VLD_STR( arg1 ) ) {
        do_races( ch, ( char * ) "" );
        return;
    }
    if ( is_number( arg1 ) && ( ra = atoi( arg1 ) ) >= 0 && ra < MAX_RACE )
        race = race_table[ra];
    else {
        race = NULL;
        for ( ra = 0; ra < MAX_RACE && race_table[ra]; ra++ )
            if ( !str_cmp( race_table[ra]->race_name, arg1 ) ) {
                race = race_table[ra];
                break;
            }
    }
    if ( !race ) {
        send_to_char( "No such race.\r\n", ch );
        return;
    }
    if ( xIS_SET( race->flags, RACE_ADVANCED ) )
        ch_printf( ch, "ADVANCED RACE: %s\r\n", race->race_name );
    else
        ch_printf( ch, "RACE: %s\r\n", race->race_name );

    ch_printf( ch, "Base: Str: %-3d Dex: %-3d Wis: %-3d Int: %-3d\r\n", race->base_str,
               race->base_dex, race->base_wis, race->base_int );
    ch_printf( ch, "      Con: %-3d Cha: %-3d Lck: %-3d\r\n", race->base_con, race->base_cha,
               race->base_lck );

    ch_printf( ch, "Hit Pts:  %-3d\tMana: %-3d\tAlign: %-4d\tAC: %-d\r\n", race->hit, race->mana,
               race->alignment, race->ac_plus );
    ch_printf( ch, "Min Align: %d\tMax Align: %-d\t\tXP Mult: %-d%%\r\n", race->minalign,
               race->maxalign, race->exp_multiplier );
    ch_printf( ch, "Height: %3d in.\t\tWeight: %4d lbs.\tHungerMod: %d\tThirstMod: %d\r\n",
               race->height, race->weight, race->hunger_mod, race->thirst_mod );
    ch_printf( ch, "SDBonus: %d\r\n", race->sdbonus );
    send_to_char( "Affected by: ", ch );
    send_to_char( !xIS_EMPTY( race->affected ) ? ext_flag_string( &race->affected, a_flags ) :
                  "Nothing", ch );
    send_to_char( "\r\n", ch );
    send_to_char( "Resistant to: ", ch );
    send_to_char( flag_string( race->resist, ris_flags ), ch );
    send_to_char( "\r\n", ch );
    send_to_char( "Susceptible to: ", ch );
    send_to_char( flag_string( race->suscept, ris_flags ), ch );
    send_to_char( "\r\n", ch );
    ch_printf( ch, "Saves: (P/D) %d (W) %d (P/P) %d (B) %d (S/S) %d\r\n", race->saving_poison_death,
               race->saving_wand, race->saving_para_petri, race->saving_breath,
               race->saving_spell_staff );
    send_to_char( "Innate Attacks: ", ch );
    send_to_char( ext_flag_string( &race->attacks, attack_flags ), ch );
    send_to_char( "\r\n", ch );
    send_to_char( "Innate Defenses: ", ch );
    send_to_char( ext_flag_string( &race->defenses, defense_flags ), ch );
    send_to_char( "\r\n", ch );
    ch_printf( ch, "Spoken Language: %s", flag_string( race->language, lang_names ) );
    send_to_char( "\r\n", ch );
    ch_printf( ch, "Race flags: %s\r\n", ext_flag_string( &race->flags, raceflags ) );
    send_to_char( "\r\n", ch );
    return;
}

int get_raceflag( char *flag )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( raceflags ) / sizeof( raceflags[0] ) ); x++ )
        if ( !str_cmp( flag, raceflags[x] ) )
            return x;
    return -1;
}

/*
int get_adv_class(int race)
{
  int x;

  for(x=0;x<MAX_CLASS;x++)
    if(race_table[race] && class_table[x] && !str_cmp(race_table[race]->race_name, class_table[x]->who_name))
      return x;

  return -1;
}
*/
