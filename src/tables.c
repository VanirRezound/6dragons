 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Table load/save module                                                *
 ***************************************************************************/

#include <limits.h>
#include <string.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/languages.h"
#include "h/key.h"

/* global variables */
int                     top_sn;
int                     top_herb;

SKILLTYPE              *skill_table[MAX_SKILL];
SKILLTYPE              *herb_table[MAX_HERB];
SKILLTYPE              *disease_table[MAX_DISEASE];

LANG_DATA              *first_lang;
LANG_DATA              *last_lang;

const char             *const skill_tname[] =
    { "unknown", "Spell", "Skill", "Weapon", "Tongue", "Racial", "Unused", "Song" };

/* Function used by qsort to sort skills */
int skill_comp( SKILLTYPE ** sk1, SKILLTYPE ** sk2 )
{
    SKILLTYPE              *skill1 = ( *sk1 );
    SKILLTYPE              *skill2 = ( *sk2 );

    if ( !skill1 && !skill2 )
        return 0;
    if ( !skill1 && skill2 )
        return 1;
    if ( skill1 && !skill2 )
        return -1;
    if ( skill1->type < skill2->type )
        return -1;
    if ( skill1->type > skill2->type )
        return 1;
    return strcmp( skill1->name, skill2->name );
}

/* Sort the skill table with qsort */
void sort_skill_table(  )
{
    log_string( "Sorting skill table..." );
    qsort( &skill_table[1], top_sn - 1, sizeof( SKILLTYPE * ),
           ( int ( * )( const void *, const void * ) ) skill_comp );
}

/* Remap slot numbers to sn values */
void remap_slot_numbers(  )
{
    SKILLTYPE              *skill;
    SMAUG_AFF              *aff;
    char                    tmp[32];
    int                     sn;

    log_string( "Remapping slots to sns" );
    for ( sn = 0; sn <= top_sn; sn++ ) {
        if ( ( skill = skill_table[sn] ) != NULL ) {
            for ( aff = skill->affects; aff; aff = aff->next )
                if ( aff->location == APPLY_WEAPONSPELL || aff->location == APPLY_WEARSPELL
                     || aff->location == APPLY_REMOVESPELL || aff->location == APPLY_STRIPSN
                     || aff->location == APPLY_RECURRINGSPELL ) {
                    snprintf( tmp, 32, "%d", slot_lookup( atoi( aff->modifier ) ) );
                    STRFREE( aff->modifier );
                    aff->modifier = STRALLOC( tmp );
                }
        }
    }
}

/* Write skill data to a file */
void fwrite_skill( FILE * fpout, SKILLTYPE * skill )
{
    SMAUG_AFF              *aff;
    int                     modifier;

    fprintf( fpout, "Name         %s~\n", skill->name );
    fprintf( fpout, "Type         %s\n", skill_tname[skill->type] );
    fprintf( fpout, "Info         %d\n", skill->info );
    fprintf( fpout, "Flags        %s\n", print_bitvector( &skill->flags ) );
    if ( skill->target )
        fprintf( fpout, "Target       %d\n", skill->target );
    if ( skill->minimum_position )
        fprintf( fpout, "Minpos       %d\n", skill->minimum_position + 100 );
    if ( skill->min_dist )
        fprintf( fpout, "Min_dist     %d\n", skill->min_dist );
    if ( skill->max_dist )
        fprintf( fpout, "Max_dist     %d\n", skill->max_dist );
    if ( skill->spell_sector )
        fprintf( fpout, "Ssector      %d\n", skill->spell_sector );
    if ( skill->saves )
        fprintf( fpout, "Saves        %d\n", skill->saves );
    if ( skill->slot )
        fprintf( fpout, "Slot         %d\n", skill->slot );
    if ( skill->min_mana )
        fprintf( fpout, "Mana         %d\n", skill->min_mana );
    if ( skill->beats )
        fprintf( fpout, "Rounds       %d\n", skill->beats );
    if ( skill->range )
        fprintf( fpout, "Range        %d\n", skill->range );
    if ( skill->skill_fun )
        fprintf( fpout, "Code         %s\n", skill_name( skill->skill_fun ) );
    else if ( skill->spell_fun )
        fprintf( fpout, "Code         %s\n", spell_name( skill->spell_fun ) );
    if ( VLD_STR( skill->noun_damage ) )
        fprintf( fpout, "Dammsg       %s~\n", skill->noun_damage );
    if ( VLD_STR( skill->msg_off ) )
        fprintf( fpout, "Wearoff      %s~\n", skill->msg_off );
    if ( VLD_STR( skill->hit_char ) )
        fprintf( fpout, "Hitchar      %s~\n", skill->hit_char );
    if ( VLD_STR( skill->hit_vict ) )
        fprintf( fpout, "Hitvict      %s~\n", skill->hit_vict );
    if ( VLD_STR( skill->hit_room ) )
        fprintf( fpout, "Hitroom      %s~\n", skill->hit_room );
    if ( VLD_STR( skill->hit_dest ) )
        fprintf( fpout, "Hitdest      %s~\n", skill->hit_dest );
    if ( VLD_STR( skill->miss_char ) )
        fprintf( fpout, "Misschar     %s~\n", skill->miss_char );
    if ( VLD_STR( skill->miss_vict ) )
        fprintf( fpout, "Missvict     %s~\n", skill->miss_vict );
    if ( VLD_STR( skill->miss_room ) )
        fprintf( fpout, "Missroom     %s~\n", skill->miss_room );
    if ( VLD_STR( skill->die_char ) )
        fprintf( fpout, "Diechar      %s~\n", skill->die_char );
    if ( VLD_STR( skill->die_vict ) )
        fprintf( fpout, "Dievict      %s~\n", skill->die_vict );
    if ( VLD_STR( skill->die_room ) )
        fprintf( fpout, "Dieroom      %s~\n", skill->die_room );
    if ( VLD_STR( skill->imm_char ) )
        fprintf( fpout, "Immchar      %s~\n", skill->imm_char );
    if ( VLD_STR( skill->imm_vict ) )
        fprintf( fpout, "Immvict      %s~\n", skill->imm_vict );
    if ( VLD_STR( skill->imm_room ) )
        fprintf( fpout, "Immroom      %s~\n", skill->imm_room );
    if ( VLD_STR( skill->dice ) )
        fprintf( fpout, "Dice         %s~\n", skill->dice );
    if ( skill->value )
        fprintf( fpout, "Value        %d\n", skill->value );
    if ( skill->difficulty )
        fprintf( fpout, "Difficulty   %d\n", skill->difficulty );
/*
  if(skill->trade)
    fprintf(fpout, "Trade   %d\n", skill->trade);
*/
    if ( skill->participants )
        fprintf( fpout, "Participants %d\n", skill->participants );
    if ( VLD_STR( skill->components ) )
        fprintf( fpout, "Components   %s~\n", skill->components );
    if ( VLD_STR( skill->teachers ) )
        fprintf( fpout, "Teachers     %s~\n", skill->teachers );
    if ( skill->verbal )
        fprintf( fpout, "Verbal       %d\n", skill->verbal );
    if ( skill->somatic )
        fprintf( fpout, "Somatic      %d\n", skill->somatic );
    for ( aff = skill->affects; aff; aff = aff->next ) {
        fprintf( fpout, "Affect       '%s' %d ", aff->duration, aff->location );
        modifier = atoi( aff->modifier );
        if ( ( aff->location == APPLY_WEAPONSPELL || aff->location == APPLY_WEARSPELL
               || aff->location == APPLY_REMOVESPELL || aff->location == APPLY_STRIPSN
               || aff->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( modifier ) )
            fprintf( fpout, "'%d' ", skill_table[modifier]->slot );
        else
            fprintf( fpout, "'%s' ", aff->modifier );
        fprintf( fpout, "%d\n", aff->bitvector );
    }
    int                     y;
    int                     min = 1000;

    for ( y = 0; y < MAX_CLASS; y++ )
        if ( skill->skill_level[y] < min )
            min = skill->skill_level[y];
    fprintf( fpout, "Minlevel     %d\n", min );
    min = 1000;
    for ( y = 0; y < MAX_RACE; y++ )
        if ( skill->race_level[y] < min )
            min = skill->race_level[y];
    fprintf( fpout, "End\n\n" );
}

/* Save the skill table to disk */
void save_skill_table(  )
{
    int                     x;
    FILE                   *fpout;

    if ( ( fpout = FileOpen( SKILL_FILE, "w" ) ) == NULL ) {
        perror( SKILL_FILE );
        bug( "Cannot open %s for writting", SKILL_FILE );
        return;
    }
    for ( x = 0; x < top_sn; x++ ) {
        if ( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
            break;
        fprintf( fpout, "#SKILL\n" );
        fwrite_skill( fpout, skill_table[x] );
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

/* Save the herb table to disk */
void save_herb_table(  )
{
    int                     x;
    FILE                   *fpout;

    if ( ( fpout = FileOpen( HERB_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open %s for writting", HERB_FILE );
        perror( HERB_FILE );
        return;
    }
    for ( x = 0; x < top_herb; x++ ) {
        if ( !herb_table[x]->name || herb_table[x]->name[0] == '\0' )
            break;
        fprintf( fpout, "#HERB\n" );
        fwrite_skill( fpout, herb_table[x] );
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

/* Save the socials to disk */
void save_socials(  )
{
    FILE                   *fpout;
    SOCIALTYPE             *social;
    int                     x;

    if ( ( fpout = FileOpen( SOCIAL_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open %s for writting", SOCIAL_FILE );
        perror( SOCIAL_FILE );
        return;
    }
    for ( x = 0; x < 27; x++ ) {
        for ( social = social_index[x]; social; social = social->next ) {
            if ( !VLD_STR( social->name ) ) {
                bug( "Save_socials: blank social in hash bucket %d", x );
                continue;
            }
            fprintf( fpout, "#SOCIAL\n" );
            fprintf( fpout, "Name        %s~\n", social->name );
            if ( VLD_STR( social->char_no_arg ) )
                fprintf( fpout, "CharNoArg   %s~\n", social->char_no_arg );
            if ( VLD_STR( social->others_no_arg ) )
                fprintf( fpout, "OthersNoArg %s~\n", social->others_no_arg );
            if ( VLD_STR( social->char_found ) )
                fprintf( fpout, "CharFound   %s~\n", social->char_found );
            if ( VLD_STR( social->others_found ) )
                fprintf( fpout, "OthersFound %s~\n", social->others_found );
            if ( VLD_STR( social->vict_found ) )
                fprintf( fpout, "VictFound   %s~\n", social->vict_found );
            if ( VLD_STR( social->char_auto ) )
                fprintf( fpout, "CharAuto    %s~\n", social->char_auto );
            if ( VLD_STR( social->others_auto ) )
                fprintf( fpout, "OthersAuto  %s~\n", social->others_auto );
            fprintf( fpout, "End\n\n" );
        }
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

int get_skill( char *skilltype )
{
    if ( !str_cmp( skilltype, "Spell" ) )
        return SKILL_SPELL;
    if ( !str_cmp( skilltype, "Skill" ) )
        return SKILL_SKILL;
    if ( !str_cmp( skilltype, "Weapon" ) )
        return SKILL_WEAPON;
    if ( !str_cmp( skilltype, "Tongue" ) )
        return SKILL_TONGUE;
/*
  if(!str_cmp(skilltype, "Trade"))
    return SKILL_TRADE;
*/
    if ( !str_cmp( skilltype, "Song" ) )
        return SKILL_SONG;

    return SKILL_UNKNOWN;
}

/* Save the commands to disk
 * Added flags Aug 25, 1997 --Shaddai
 */
void save_commands(  )
{
    FILE                   *fpout;
    CMDTYPE                *command;
    int                     x;

    if ( ( fpout = FileOpen( COMMAND_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open %s for writing", COMMAND_FILE );
        perror( COMMAND_FILE );
        return;
    }
    for ( x = 0; x < 126; x++ ) {
        for ( command = command_hash[x]; command; command = command->next ) {
            if ( !VLD_STR( command->name ) ) {
                bug( "Save_commands: blank command in hash bucket %d", x );
                continue;
            }
            fprintf( fpout, "#COMMAND\n" );
            fprintf( fpout, "Name        %s~\n", command->name );
            fprintf( fpout, "Code        %s\n", skill_name( command->do_fun ) );
            if ( command->position < 100 )
                fprintf( fpout, "Position    %d\n", command->position + 100 );
            else
                fprintf( fpout, "Position    %d\n", command->position );
            fprintf( fpout, "Level       %d\n", command->level );
            fprintf( fpout, "CShow       %d\n", command->cshow );
            fprintf( fpout, "Log         %d\n", command->log );
            if ( command->flags )
                fprintf( fpout, "Flags       %d\n", command->flags );
            fprintf( fpout, "End\n\n" );
        }
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

SKILLTYPE              *fread_skill( FILE * fp )
{
    const char             *word;
    bool                    fMatch;
    bool                    got_info = FALSE;
    SKILLTYPE              *skill;
    int                     x;

    CREATE( skill, SKILLTYPE, 1 );
    skill->slot = 0;
    skill->min_mana = 0;
    for ( x = 0; x < MAX_CLASS; x++ ) {
        skill->skill_level[x] = LEVEL_IMMORTAL;
        skill->skill_adept[x] = 95;
    }
    for ( x = 0; x < MAX_RACE; x++ ) {
        skill->race_level[x] = LEVEL_IMMORTAL;
        skill->race_adept[x] = 95;
    }
    skill->target = 0;
    skill->skill_fun = NULL;
    skill->spell_fun = NULL;
    skill->spell_sector = 0;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                if ( !str_cmp( word, "Affect" ) ) {
                    SMAUG_AFF              *aff;

                    CREATE( aff, SMAUG_AFF, 1 );
                    aff->duration = STRALLOC( fread_word( fp ) );
                    aff->location = fread_number( fp );
                    aff->modifier = STRALLOC( fread_word( fp ) );
                    aff->bitvector = fread_number( fp );
                    if ( !got_info ) {
                        for ( x = 0; x < 32; x++ ) {
                            if ( IS_SET( aff->bitvector, 1 << x ) ) {
                                aff->bitvector = x;
                                break;
                            }
                        }
                        if ( x == 32 )
                            aff->bitvector = -1;
                    }
                    aff->next = skill->affects;
                    skill->affects = aff;
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'C':
                if ( !str_cmp( word, "Class" ) ) {
                    int                     Class = fread_number( fp );

                    skill->skill_level[Class] = fread_number( fp );
                    skill->skill_adept[Class] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                // Added in support for second and third classes here. -Taon
                if ( !str_cmp( word, "Secondclass" ) ) {
                    int                     secondclass = fread_number( fp );

                    skill->skill_level[secondclass] = fread_number( fp );
                    skill->skill_adept[secondclass] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !str_cmp( word, "Thirdclass" ) ) {
                    int                     thirdclass = fread_number( fp );

                    skill->skill_level[thirdclass] = fread_number( fp );
                    skill->skill_adept[thirdclass] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !str_cmp( word, "Code" ) ) {
                    SPELL_FUN              *spellfun;
                    DO_FUN                 *dofun;
                    char                   *w = fread_word( fp );

                    fMatch = TRUE;
                    if ( ( spellfun = spell_function( w ) ) != spell_notfound ) {
                        skill->spell_fun = spellfun;
                        skill->skill_fun = NULL;
                    }
                    else if ( ( dofun = skill_function( w ) ) != skill_notfound ) {
                        skill->skill_fun = dofun;
                        skill->spell_fun = NULL;
                    }
                    else {
                        bug( "fread_skill: unknown skill/spell %s", w );
                        skill->spell_fun = spell_null;
                    }
                    break;
                }
                KEY( "Code", skill->spell_fun, spell_function( fread_word( fp ) ) );
                KEY( "Components", skill->components, fread_string( fp ) );
                break;
            case 'D':
                KEY( "Dammsg", skill->noun_damage, fread_string( fp ) );
                KEY( "Dice", skill->dice, fread_string( fp ) );
                KEY( "Diechar", skill->die_char, fread_string( fp ) );
                KEY( "Dieroom", skill->die_room, fread_string( fp ) );
                KEY( "Dievict", skill->die_vict, fread_string( fp ) );
                KEY( "Difficulty", skill->difficulty, fread_number( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( skill->saves != 0 && SPELL_SAVE( skill ) == SE_NONE ) {
                        bug( "fread_skill(%s):  Has saving throw (%d) with no saving effect.",
                             skill->name, skill->saves );
                        SET_SSAV( skill, SE_NEGATE );
                    }
                    return skill;
                }
                break;
            case 'F':
                if ( !str_cmp( word, "Flags" ) ) {
                    skill->flags = fread_bitvector( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'G':
                break;
            case 'H':
                KEY( "Hitchar", skill->hit_char, fread_string( fp ) );
                KEY( "Hitdest", skill->hit_dest, fread_string( fp ) );
                KEY( "Hitroom", skill->hit_room, fread_string( fp ) );
                KEY( "Hitvict", skill->hit_vict, fread_string( fp ) );
                break;
            case 'I':
                KEY( "Immchar", skill->imm_char, fread_string( fp ) );
                KEY( "Immroom", skill->imm_room, fread_string( fp ) );
                KEY( "Immvict", skill->imm_vict, fread_string( fp ) );
                if ( !str_cmp( word, "Info" ) ) {
                    skill->info = fread_number( fp );
                    got_info = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'M':
                KEY( "Mana", skill->min_mana, fread_number( fp ) );
                if ( !str_cmp( word, "Minlevel" ) ) {
                    fread_to_eol( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Max_dist" ) ) {
                    skill->max_dist = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Min_dist" ) ) {
                    skill->min_dist = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Minpos" ) ) {
                    fMatch = TRUE;
                    skill->minimum_position = fread_number( fp );
                    if ( skill->minimum_position < 100 ) {
                        switch ( skill->minimum_position ) {
                            default:
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                                break;
                            case 5:
                                skill->minimum_position = 6;
                                break;
                            case 6:
                                skill->minimum_position = 8;
                                break;
                            case 7:
                                skill->minimum_position = 9;
                                break;
                            case 8:
                                skill->minimum_position = 12;
                                break;
                            case 9:
                                skill->minimum_position = 13;
                                break;
                            case 10:
                                skill->minimum_position = 14;
                                break;
                            case 11:
                                skill->minimum_position = 15;
                                break;
                        }
                    }
                    else
                        skill->minimum_position -= 100;
                    break;
                }
                KEY( "Misschar", skill->miss_char, fread_string( fp ) );
                KEY( "Missroom", skill->miss_room, fread_string( fp ) );
                KEY( "Missvict", skill->miss_vict, fread_string( fp ) );
                break;
            case 'N':
                KEY( "Name", skill->name, fread_string( fp ) );
                break;
            case 'P':
                KEY( "Participants", skill->participants, fread_number( fp ) );
                break;
            case 'R':
                KEY( "Range", skill->range, fread_number( fp ) );
                KEY( "Rounds", skill->beats, fread_number( fp ) );
                if ( !str_cmp( word, "Race" ) ) {
                    int                     race = fread_number( fp );

                    skill->race_level[race] = fread_number( fp );
                    skill->race_adept[race] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'S':
                KEY( "Saves", skill->saves, fread_number( fp ) );
                KEY( "Slot", skill->slot, fread_number( fp ) );
                KEY( "Somatic", skill->somatic, fread_number( fp ) );
                KEY( "Ssector", skill->spell_sector, fread_number( fp ) );
                break;
            case 'T':
                KEY( "Target", skill->target, fread_number( fp ) );
                KEY( "Teachers", skill->teachers, fread_string( fp ) );
//        KEY("Trade", skill->trade, fread_number(fp));
                KEY( "Type", skill->type, get_skill( fread_word( fp ) ) );
                break;
            case 'V':
                KEY( "Value", skill->value, fread_number( fp ) );
                KEY( "Verbal", skill->verbal, fread_number( fp ) );
                break;
            case 'W':
                KEY( "Wearoff", skill->msg_off, fread_string( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_skill: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void load_skill_table(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( SKILL_FILE, "r" ) ) != NULL ) {
        top_sn = 0;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_skill_table: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "SKILL" ) ) {
                if ( top_sn >= MAX_SKILL ) {
                    bug( "load_skill_table: more skills than MAX_SKILL %d", MAX_SKILL );
                    FileClose( fp );
                    return;
                }
                skill_table[top_sn++] = fread_skill( fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_skill_table: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        perror( SKILL_FILE );
        bug( "%s", "Cannot open skills.dat" );
        exit( 0 );
    }
}

void load_herb_table(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( HERB_FILE, "r" ) ) != NULL ) {
        top_herb = 0;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_herb_table: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "HERB" ) ) {
                if ( top_herb >= MAX_HERB ) {
                    bug( "load_herb_table: more herbs than MAX_HERB %d", MAX_HERB );
                    FileClose( fp );
                    return;
                }
                herb_table[top_herb++] = fread_skill( fp );
                if ( herb_table[top_herb - 1]->slot == 0 )
                    herb_table[top_herb - 1]->slot = top_herb - 1;
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_herb_table: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        bug( "%s", "Cannot open herbs.dat" );
        exit( 0 );
    }
}

void fread_social( FILE * fp )
{
    const char             *word;
    bool                    fMatch;
    SOCIALTYPE             *social;

    CREATE( social, SOCIALTYPE, 1 );
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'C':
                KEY( "CharNoArg", social->char_no_arg, fread_string( fp ) );
                KEY( "CharFound", social->char_found, fread_string( fp ) );
                KEY( "CharAuto", social->char_auto, fread_string( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !social->name ) {
                        bug( "%s", "Fread_social: Name not found" );
                        free_social( social );
                        return;
                    }
                    if ( !social->char_no_arg ) {
                        bug( "%s", "Fread_social: CharNoArg not found" );
                        free_social( social );
                        return;
                    }
                    add_social( social );
                    return;
                }
                break;
            case 'N':
                KEY( "Name", social->name, fread_string( fp ) );
                break;
            case 'O':
                KEY( "OthersNoArg", social->others_no_arg, fread_string( fp ) );
                KEY( "OthersFound", social->others_found, fread_string( fp ) );
                KEY( "OthersAuto", social->others_auto, fread_string( fp ) );
                break;
            case 'V':
                KEY( "VictFound", social->vict_found, fread_string( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_social: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void load_socials(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( SOCIAL_FILE, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_socials: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "SOCIAL" ) ) {
                fread_social( fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_socials: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        bug( "%s", "Cannot open socials.dat" );
        exit( 0 );
    }
}

/*
 *  Added the flags Aug 25, 1997 --Shaddai
 */

void fread_command( FILE * fp )
{
    const char             *word;
    bool                    fMatch;
    CMDTYPE                *command;

    CREATE( command, CMDTYPE, 1 );
    command->lag_count = 0;                            /* can't have caused lag yet... FB 
                                                        */
    command->flags = 0;                                /* Default to no flags set */
    command->cshow = 1;                                /* Default to command shown -
                                                        * Zarius */

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'C':
                KEY( "Code", command->do_fun, skill_function( fread_word( fp ) ) );
                KEY( "CShow", command->cshow, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !command->name ) {
                        bug( "%s", "Fread_command: Name not found" );
                        free_command( command );
                        return;
                    }
                    if ( !command->do_fun ) {
                        bug( "%s", "Fread_command: Function not found" );
                        free_command( command );
                        return;
                    }
                    add_command( command );
                    return;
                }
                break;

            case 'F':
                KEY( "Flags", command->flags, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", command->level, fread_number( fp ) );
                KEY( "Log", command->log, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", command->name, fread_string( fp ) );
                break;

            case 'P':
                /*
                 * KEY("Position",  command->position,  fread_number(fp)); 
                 */
                if ( !str_cmp( word, "Position" ) ) {
                    fMatch = TRUE;
                    command->position = fread_number( fp );
                    if ( command->position < 100 ) {
                        switch ( command->position ) {
                            default:
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                                break;
                            case 5:
                                command->position = 6;
                                break;
                            case 6:
                                command->position = 8;
                                break;
                            case 7:
                                command->position = 9;
                                break;
                            case 8:
                                command->position = 12;
                                break;
                            case 9:
                                command->position = 13;
                                break;
                            case 10:
                                command->position = 14;
                                break;
                            case 11:
                                command->position = 15;
                                break;
                        }
                    }
                    else
                        command->position -= 100;
                    break;
                }
                break;

        }

        if ( !fMatch ) {
            bug( "Fread_command: no match: %s", word );
        }
    }
}

void load_commands(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( COMMAND_FILE, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_commands: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "COMMAND" ) ) {
                fread_command( fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_commands: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        bug( "%s", "Cannot open commands.dat" );
        exit( 0 );
    }

}

/*
 * Tongues / Languages loading/saving functions   -Altrag
 */
void fread_cnv( FILE * fp, LCNV_DATA ** first_cnv, LCNV_DATA ** last_cnv )
{
    LCNV_DATA              *cnv;
    char                    letter;

    for ( ;; ) {
        letter = fread_letter( fp );
        if ( letter == '~' || letter == EOF )
            break;
        ungetc( letter, fp );
        CREATE( cnv, LCNV_DATA, 1 );

        cnv->old = STRALLOC( fread_word( fp ) );
        cnv->olen = strlen( cnv->old );
        cnv->lnew = STRALLOC( fread_word( fp ) );
        cnv->nlen = strlen( cnv->lnew );
        fread_to_eol( fp );
        LINK( cnv, *first_cnv, *last_cnv, next, prev );
    }
}

void load_tongues(  )
{
    FILE                   *fp;
    LANG_DATA              *lng;
    char                   *word;
    char                    letter;

    if ( !( fp = FileOpen( TONGUE_FILE, "r" ) ) ) {
        perror( "Load_tongues" );
        return;
    }
    for ( ;; ) {
        letter = fread_letter( fp );
        if ( letter == EOF )
            return;
        else if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }
        else if ( letter != '#' ) {
            bug( "Letter '%c' not #.", letter );
            exit( 0 );
        }
        word = fread_word( fp );
        if ( !str_cmp( word, "end" ) )
            break;
        fread_to_eol( fp );
        CREATE( lng, LANG_DATA, 1 );
        if ( VLD_STR( word ) )
            lng->name = STRALLOC( word );
        fread_cnv( fp, &lng->first_precnv, &lng->last_precnv );
        lng->alphabet = fread_string( fp );
        fread_cnv( fp, &lng->first_cnv, &lng->last_cnv );
        fread_to_eol( fp );
        LINK( lng, first_lang, last_lang, next, prev );
    }
    FileClose( fp );
    fp = NULL;
}

void fwrite_langs( void )
{
    FILE                   *fp;
    LANG_DATA              *lng;
    LCNV_DATA              *cnv;

    if ( !( fp = FileOpen( TONGUE_FILE, "w" ) ) ) {
        perror( "fwrite_langs" );
        return;
    }
    for ( lng = first_lang; lng; lng = lng->next ) {
        fprintf( fp, "#%s\n", lng->name );
        for ( cnv = lng->first_precnv; cnv; cnv = cnv->next )
            fprintf( fp, "'%s' '%s'\n", cnv->old, cnv->lnew );
        fprintf( fp, "~\n%s~\n", lng->alphabet );
        for ( cnv = lng->first_cnv; cnv; cnv = cnv->next )
            fprintf( fp, "'%s' '%s'\n", cnv->old, cnv->lnew );
        fprintf( fp, "\n" );
    }
    fprintf( fp, "#end\n\n" );
    FileClose( fp );
}

void fread_bank( FILE * fp )
{
    char                    buf[MAX_STRING_LENGTH];
    const char             *word;
    bool                    fMatch;
    BANK_DATA              *bank;

    CREATE( bank, BANK_DATA, 1 );
    bank->copper = 0;
    bank->bronze = 0;
    bank->silver = 0;
    bank->gold = 0;
    bank->lastused = 0;
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'B':
                KEY( "Bronze", bank->bronze, fread_number( fp ) );
                break;

            case 'C':
                KEY( "Copper", bank->copper, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    sysdata.scount += 1;
                    if ( !bank->lastused )
                        bank->lastused = current_time;
                    if ( !bank->name ) {
                        bug( "Fread_bank: Name not found" );
                        free_bank( bank );
                        return;
                    }
                    if ( ( current_time - bank->lastused ) / 31622400 ) {   /* If not
                                                                             * used for
                                                                             * 366 days
                                                                             * get * rid 
                                                                             * of it */
                        bug( "%s: Deleting bank %s because it hasn't been used in at least 366 days.", __FUNCTION__, bank->name );
                        return;
                    }
                    add_bank( bank );
                    return;
                }
                break;

            case 'G':
                KEY( "Gold", bank->gold, fread_number( fp ) );
                break;

            case 'L':
                KEY( "LastUsed", bank->lastused, fread_time( fp ) );
                break;

            case 'N':
                KEY( "Name", bank->name, fread_string( fp ) );
                break;

            case 'P':
                KEY( "Password", bank->password, fread_string( fp ) );
                break;

            case 'S':
                KEY( "Silver", bank->silver, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_bank: no match: %s", word );
        }
    }
}

void load_bank(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( BANK_FILE, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "Load_bank: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "BANK" ) ) {
                fread_bank( fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "Load_bank:	bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        bug( "Cannot open	bank.dat" );
        exit( 0 );
    }
}

void save_bank( void )
{
    FILE                   *fpout;
    BANK_DATA              *bank;
    int                     x;

    if ( !( fpout = FileOpen( BANK_FILE, "w" ) ) ) {
        bug( "Cannot open	bank.dat for writting" );
        perror( BANK_FILE );
        return;
    }

    for ( x = 0; x < 27; x++ ) {
        for ( bank = bank_index[x]; bank; bank = bank->next ) {
            if ( !bank->name || bank->name[0] == '\0' ) {
                bug( "Save_bank: blank bank in hash	bucket %d", x );
                continue;
            }
            fprintf( fpout, "#BANK\n" );
            fprintf( fpout, "Name        %s~\n", bank->name );
            fprintf( fpout, "Password    %s~\n", bank->password );
            fprintf( fpout, "LastUsed    %ld\n", bank->lastused );
            if ( bank->bronze )
                fprintf( fpout, "Bronze      %d\n", bank->bronze );
            if ( bank->copper )
                fprintf( fpout, "Copper      %d\n", bank->copper );
            if ( bank->gold )
                fprintf( fpout, "Gold        %d\n", bank->gold );
            if ( bank->silver )
                fprintf( fpout, "Silver      %d\n", bank->silver );
            fprintf( fpout, "End\n\n" );
        }
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}
