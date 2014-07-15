 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Character loading and saving module                                   *
 ***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifndef WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include "h/mud.h"
#include "h/files.h"
#include "h/hometowns.h"
#include "h/city.h"
#include "h/clans.h"
#include "h/channels.h"
#include "h/languages.h"
#include "h/polymorph.h"
#include "h/key.h"

int                     board_lookup( const char *name );
void                    link_chquest( CHAR_DATA *ch, CHQUEST_DATA * chquest );
MUD_CHANNEL            *find_channel( const char *name );
PKILLED_DATA           *new_pkill( void );
PKILLED_DATA           *has_pkilled( CHAR_DATA *ch, char *name );
void                    combine_affects( OBJ_DATA *obj );

/*
 * Increment with every major format change.
 * Upped to 5 for addition of new Age Setup. -Kayle
 */
#define SAVEVERSION 5

/*
 * Array to keep track of equipment temporarily.  -Thoric
 */
OBJ_DATA               *save_equipment[MAX_WEAR][MAX_LAYERS];
CHAR_DATA              *quitting_char,
                       *loading_char,
                       *saving_char;

int                     file_ver;

/*
 * Externals
 */
extern FILE            *fpArea;
extern char             strArea[MIL];
extern int              falling;
void                    fwrite_comments( CHAR_DATA *ch, FILE * fp );
void                    fread_comment( CHAR_DATA *ch, FILE * fp );
void                    fwrite_variables( CHAR_DATA *ch, FILE * fp );
void                    fread_variable( CHAR_DATA *ch, FILE * fp );

/*
 * Array of containers read for proper re-nesting of objects.
 */
static OBJ_DATA        *rgObjNest[MAX_NEST];

/*
 * Local functions.
 */
void fwrite_char        args( ( CHAR_DATA *ch, FILE * fp ) );
void fread_char         args( ( CHAR_DATA *ch, FILE * fp, bool preload, bool copyover ) );
void write_corpses      args( ( CHAR_DATA *ch, char *name, OBJ_DATA *objrem ) );
void                    removename( char **list, const char *name );

/* Alias code definition */
ALIAS_DATA             *pal;

#ifdef WIN32                                           /* NJG */
UINT                    timer_code = 0;                /* needed to kill the timer */

/* Note: need to include: WINMM.LIB to link to timer functions */
void                    caught_alarm(  );
void CALLBACK alarm_handler( UINT IDEvent,             /* identifies timer event */
                             UINT uReserved,           /* not used */
                             DWORD dwUser,             /* application-defined instance
                                                        * data */
                             DWORD dwReserved1,        /* not used */
                             DWORD dwReserved2 )
{                                                      /* not used */
    caught_alarm(  );
}

void kill_timer(  )
{
    if ( timer_code )
        timeKillEvent( timer_code );
    timer_code = 0;
}

#endif

/*
 * Un-equip character before saving to ensure proper  -Thoric
 * stats are saved in case of changes to or removal of EQ
 */
void de_equip_char( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    int                     x,
                            y;

    for ( x = 0; x < MAX_WEAR; x++ )
        for ( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;
    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->wear_loc > -1 && obj->wear_loc < MAX_WEAR ) {
            if ( get_trust( ch ) >= obj->level ) {
                for ( x = 0; x < MAX_LAYERS; x++ )
                    if ( !save_equipment[obj->wear_loc][x] ) {
                        save_equipment[obj->wear_loc][x] = obj;
                        break;
                    }
                if ( x == MAX_LAYERS ) {
                    bug( "%s had on more than %d layers of clothing in one location (%d): %s",
                         ch->name, MAX_LAYERS, obj->wear_loc, obj->name );
                }
            }
            else {
                bug( "%s had on %s:  ch->level = %d  obj->level = %d", ch->name, obj->name,
                     ch->level, obj->level );
            }
            unequip_char( ch, obj );
        }
}

/*
 * Re-equip character     -Thoric
 */
void re_equip_char( CHAR_DATA *ch )
{
    int                     x,
                            y;

    for ( x = 0; x < MAX_WEAR; x++ )
        for ( y = 0; y < MAX_LAYERS; y++ )
            if ( save_equipment[x][y] != NULL ) {
                if ( quitting_char != ch )
                    equip_char( ch, save_equipment[x][y], x );
                save_equipment[x][y] = NULL;
            }
            else
                break;
}

short find_old_age( CHAR_DATA *ch )
{
    short                   age;

    if ( IS_NPC( ch ) )
        return -1;

    age = ch->played / 86400;                          /* Calculate realtime number of
                                                        * days played */

    age = age / 7;                                     /* Calculates rough estimate on
                                                        * number of mud years played */

    age += 17;                                         /* Add 17 years, new characters
                                                        * begin at 17. */

    ch->pcdata->day = ( number_range( 1, sysdata.dayspermonth ) - 1 );  /* Assign random
                                                                         * day of birth */
    ch->pcdata->month = ( number_range( 1, sysdata.monthsperyear ) - 1 );   /* Assign
                                                                             * random
                                                                             * month of
                                                                             * birth */
    ch->pcdata->year = time_info.year - age;           /* Assign birth year based on
                                                        * calculations above */

    return age;
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char                    strsave[MIL];
    char                    strback[MIL];
    FILE                   *fp;

    if ( !ch ) {
        bug( "%s", "Save_char_obj: null ch!" );
        return;
    }

    if ( IS_NPC( ch ) || ch->level < 1 )
        return;

    if ( ch->level < 2 && ( !ch->desc || ch->desc->connected != CON_PLAYING ) )
        return;

    if ( ch )
        update_roster( ch );

    if ( ch )
        update_rollcall( ch );

    saving_char = ch;

    if ( ch->desc && ch->desc->original )
        ch = ch->desc->original;

    de_equip_char( ch );

    ch->save_time = current_time;
    snprintf( strsave, MIL, "%s%c/%s", PLAYER_DIR, tolower( ch->pcdata->filename[0] ),
              capitalize( ch->pcdata->filename ) );

    /*
     * Save immortal stats, level & vnums for wizlist  -Thoric
     * and do_vnums command
     *
     * Also save the player flags so we the wizlist builder can see
     * who is a guest and who is retired.
     */
    if ( ch->level >= LEVEL_IMMORTAL ) {
        snprintf( strback, MIL, "%s%s", STAFF_DIR, capitalize( ch->pcdata->filename ) );

        if ( ( fp = FileOpen( strback, "w" ) ) == NULL ) {
            perror( strsave );
            bug( "%s", "Save_staff_level: FileOpen" );
        }
        else {
            fprintf( fp, "Level        %d\n", ch->level );
            fprintf( fp, "Pcflags      %d\n", ch->pcdata->flags );

            if ( ch->pcdata->alt1 )
                fprintf( fp, "Alt1       %s~\n", ch->pcdata->alt1 );
            if ( ch->pcdata->alt2 )
                fprintf( fp, "Alt2       %s~\n", ch->pcdata->alt2 );
            if ( ch->pcdata->alt3 )
                fprintf( fp, "Alt3       %s~\n", ch->pcdata->alt3 );
            if ( ch->pcdata->alt4 )
                fprintf( fp, "Alt4       %s~\n", ch->pcdata->alt4 );
            if ( ch->pcdata->alt5 )
                fprintf( fp, "Alt5       %s~\n", ch->pcdata->alt5 );
            if ( ch->pcdata->rand )
                fprintf( fp, "Rand       %s~\n", ch->pcdata->rand );
            if ( ch->pcdata->lair > 0 )
                fprintf( fp, "LAIR         %d\n", ch->pcdata->lair );
            if ( ch->pcdata->palace > 0 )
                fprintf( fp, "Palace         %d\n", ch->pcdata->palace );
            if ( ch->pcdata->extlevel > 0 )
                fprintf( fp, "extlevel         %d\n", ch->pcdata->extlevel );
            if ( ch->pcdata->tmplevel > 0 )
                fprintf( fp, "tmplevel         %d\n", ch->pcdata->tmplevel );
            if ( ch->pcdata->tmpmax_hit > 0 )
                fprintf( fp, "tmpmax_hit         %d\n", ch->pcdata->tmpmax_hit );
            if ( ch->pcdata->tmpmax_move > 0 )
                fprintf( fp, "tmpmax_move         %d\n", ch->pcdata->tmpmax_move );
            if ( ch->pcdata->tmpmax_mana > 0 )
                fprintf( fp, "tmpmax_mana         %d\n", ch->pcdata->tmpmax_mana );
            if ( ch->pcdata->homepage && ch->pcdata->homepage[0] != '\0' )
                fprintf( fp, "Homepage    %s~\n", ch->pcdata->homepage );
            if ( ch->pcdata->r_range_lo && ch->pcdata->r_range_hi )
                fprintf( fp, "RoomRange    %d %d\n", ch->pcdata->r_range_lo,
                         ch->pcdata->r_range_hi );
            if ( ch->pcdata->o_range_lo && ch->pcdata->o_range_hi )
                fprintf( fp, "ObjRange     %d %d\n", ch->pcdata->o_range_lo,
                         ch->pcdata->o_range_hi );
            if ( ch->pcdata->m_range_lo && ch->pcdata->m_range_hi )
                fprintf( fp, "MobRange     %d %d\n", ch->pcdata->m_range_lo,
                         ch->pcdata->m_range_hi );
            if ( ch->pcdata->email && ch->pcdata->email[0] != '\0' )
                fprintf( fp, "Email        %s~\n", ch->pcdata->email );
            if ( ch->pcdata->icq > 0 )
                fprintf( fp, "ICQ          %d\n", ch->pcdata->icq );
            if ( ch->pcdata->clanpoints > 0 )
                fprintf( fp, "Clanpoints          %d\n", ch->pcdata->clanpoints );
            if ( ch->pcdata->tradelevel > 0 )
                fprintf( fp, "Tradelevel          %d\n", ch->pcdata->tradelevel );
            if ( ch->pcdata->tradeclass > 0 )
                fprintf( fp, "Tradeclass          %d\n", ch->pcdata->tradeclass );
            if ( ch->pcdata->craftpoints > 0 )
                fprintf( fp, "Craftpoints          %d\n", ch->pcdata->craftpoints );
            if ( ch->pcdata->banish > 0 )
                fprintf( fp, "Banish          %d\n", ch->pcdata->banish );
            if ( ch->pcdata->rprate > 0 )
                fprintf( fp, "Rprate          %d\n", ch->pcdata->rprate );
            if ( ch->pcdata->lastrated )
                fprintf( fp, "Lastrated       %ld\n", ch->pcdata->lastrated );

            fprintf( fp, "End\n" );
            FileClose( fp );
        }
    }

    if ( ( fp = FileOpen( TEMP_FILE, "w" ) ) == NULL ) {
        perror( strsave );
        bug( "%s", "Save_char_obj: FileOpen" );
    }
    else {
        bool                    ferr;

        fwrite_char( ch, fp );
        if ( ch->morph )
            fwrite_morph_data( ch, fp );
        if ( ch->first_carrying )
            fwrite_obj( ch, ch->last_carrying, fp, 0, OS_CARRY, ch->pcdata->copyover );

        if ( sysdata.save_pets && ch->pcdata->pet )
            fwrite_mobile( fp, ch->pcdata->pet );
        if ( ch->variables )
            fwrite_variables( ch, fp );
        if ( ch->comments )                            /* comments */
            fwrite_comments( ch, fp );                 /* comments */
        fprintf( fp, "#END\n" );
        ferr = ferror( fp );
        FileClose( fp );
        if ( ferr ) {
            perror( strsave );
            bug( "Error writing temp file for %s -- not copying", strsave );
        }
        else {
            remove( strsave );
            rename( TEMP_FILE, strsave );
        }
    }

    re_equip_char( ch );

    quitting_char = NULL;
    saving_char = NULL;
}

/* Write the char. */
void fwrite_char( CHAR_DATA *ch, FILE * fp )
{
    AFFECT_DATA            *paf;
    SKILLTYPE              *skill = NULL;
    int                     sn,
                            count,
                            i;
    short                   pos;
    bool                    mounted = FALSE;

    fprintf( fp, "#PLAYER\n" );
    fprintf( fp, "Version      %d\n", SAVEVERSION );
    fprintf( fp, "Name         %s~\n", ch->name );
    if ( VLD_STR( ch->description ) )
        fprintf( fp, "Description  %s~\n", strip_cr( ch->description ) );
    fprintf( fp, "Sex          %d\n", ch->sex );
    fprintf( fp, "Class        %d\n", ch->Class );
    fprintf( fp, "Secondclass  %d\n", ch->secondclass );
    fprintf( fp, "Thirdclass   %d\n", ch->thirdclass );
    fprintf( fp, "Race         %d\n", ch->race );
    fprintf( fp, "Age          %d %d %d %d\n", ch->pcdata->age_bonus, ch->pcdata->day,
             ch->pcdata->month, ch->pcdata->year );
    fprintf( fp, "Languages    %d %d\n", ch->speaks, ch->speaking );
    fprintf( fp, "Level        %d\n", ch->level );
    fprintf( fp, "Firstlevel   %d\n", ch->firstlevel );
    fprintf( fp, "Secondlevel  %d\n", ch->secondlevel );
    fprintf( fp, "Thirdlevel   %d\n", ch->thirdlevel );
    fprintf( fp, "Played       %d\n", ch->played + ( int ) ( current_time - ch->logon ) );
    fprintf( fp, "Room         %d\n",
             ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
               && ch->was_in_room ) ? ch->was_in_room->vnum : ch->in_room->vnum );
    fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana,
             ch->move, ch->max_move );
    fprintf( fp, "BaseHp       %d \n", ch->temp_base_hit );
    fprintf( fp, "Blood        %d %d\n", ch->blood, ch->max_blood );
    fprintf( fp, "Money        " );
    for ( count = 0; count < MAX_CURR_TYPE; count++ )
        fprintf( fp, " %d", GET_MONEY( ch, count ) );
    fprintf( fp, " -1\n" );
    if ( ch->pcdata && ch->pcdata->favor )
        fprintf( fp, "Favor         %d\n", ch->pcdata->favor );
    fprintf( fp, "Exp           %d\n", ch->exp );
    fprintf( fp, "Firstexp      %d\n", ch->firstexp );
    fprintf( fp, "Secondexp     %d\n", ch->secondexp );
    fprintf( fp, "Thirdexp      %d\n", ch->thirdexp );
    fprintf( fp, "Height        %d\n", ch->height );
    fprintf( fp, "Weight        %d\n", ch->weight );

    if ( ch->pcdata && ch->pcdata->bank )
        fprintf( fp, "Bank          %s~\n", ch->pcdata->bank->name );

    if ( !xIS_EMPTY( ch->act ) ) {
        if ( xIS_SET( ch->act, ACT_MOUNTED ) ) {
            mounted = TRUE;
            xREMOVE_BIT( ch->act, ACT_MOUNTED );
        }
        fprintf( fp, "Act           %s\n", print_bitvector( &ch->act ) );
        if ( mounted )
            xSET_BIT( ch->act, ACT_MOUNTED );
    }
    if ( !xIS_EMPTY( ch->affected_by ) )
        fprintf( fp, "AffectedBy    %s\n", print_bitvector( &ch->affected_by ) );
    if ( !xIS_EMPTY( ch->no_affected_by ) )
        fprintf( fp, "NoAffectedBy  %s\n", print_bitvector( &ch->no_affected_by ) );
/*  if(!xIS_EMPTY(ch->affected_by2))
    fprintf(fp, "AffectedBy2    %s\n", print_bitvector(&ch->affected_by2));
  if(!xIS_EMPTY(ch->no_affected_by2))
    fprintf(fp, "NoAffectedBy2  %s\n", print_bitvector(&ch->no_affected_by2)); */
    if ( ch->pcdata->chan_listen )
        fprintf( fp, "Channels      %s~\n", ch->pcdata->chan_listen );
    if ( ch->desc && ch->desc->speed > 0 )
        fprintf( fp, "Speed         %d\n", ch->desc->speed );
    pos = ch->position;
    if ( pos == POS_BERSERK || pos == POS_AGGRESSIVE || pos == POS_FIGHTING || pos == POS_DEFENSIVE
         || pos == POS_EVASIVE || pos == POS_MOUNTED )
        pos = POS_STANDING;
    pos += 100;
    fprintf( fp, "Position      %d\n", pos );
    fprintf( fp, "Style         %d\n", ch->style );
    fprintf( fp, "Practice      %d\n", ch->practice );
    fprintf( fp, "SavingThrows  %d %d %d %d %d\n", ch->saving_poison_death, ch->saving_wand,
             ch->saving_para_petri, ch->saving_breath, ch->saving_spell_staff );
    fprintf( fp, "Alignment     %d\n", ch->alignment );
    fprintf( fp, "AppBlood      %d\n", ch->pcdata->apply_blood );
    fprintf( fp, "Hitroll       %d\n", ch->hitroll );
    fprintf( fp, "Damroll       %d\n", ch->damroll );
    fprintf( fp, "Armor         %d\n", ch->armor );
    if ( ch->wimpy )
        fprintf( fp, "Wimpy         %d\n", ch->wimpy );
    if ( !xIS_EMPTY( ch->deaf ) )
        fprintf( fp, "Deaf          %s\n", print_bitvector( &ch->deaf ) );
    if ( ch->resistant )
        fprintf( fp, "Resistant     %d\n", ch->resistant );
    if ( ch->no_resistant )
        fprintf( fp, "NoResistant   %d\n", ch->no_resistant );
    if ( ch->immune )
        fprintf( fp, "Immune        %d\n", ch->immune );
    if ( ch->no_immune )
        fprintf( fp, "NoImmune      %d\n", ch->no_immune );
    if ( ch->susceptible )
        fprintf( fp, "Susceptible   %d\n", ch->susceptible );
    if ( ch->no_susceptible )
        fprintf( fp, "NoSusceptible %d\n", ch->no_susceptible );
    if ( ch->pcdata && ch->pcdata->nuisance )
        fprintf( fp, "NuisanceNew   %ld %ld %d %d\n", ch->pcdata->nuisance->time,
                 ch->pcdata->nuisance->max_time, ch->pcdata->nuisance->flags,
                 ch->pcdata->nuisance->power );
    if ( ch->mental_state != -10 )
        fprintf( fp, "Mentalstate   %d\n", ch->mental_state );
    fprintf( fp, "SQL           %d\n", ch->pcdata->sqlnumber );
    if ( VLD_STR( ch->pcdata->sqlpass ) )
        fprintf( fp, "SQLpass       %s~\n", ch->pcdata->sqlpass );
    fprintf( fp, "Password      %s~\n", ch->pcdata->pwd );
    if ( VLD_STR( ch->pcdata->rank ) )
        fprintf( fp, "Rank          %s~\n", ch->pcdata->rank );
    if ( VLD_STR( ch->pcdata->bestowments ) )
        fprintf( fp, "Bestowments   %s~\n", ch->pcdata->bestowments );
    fprintf( fp, "Title         %s~\n", ch->pcdata->title );
    if ( VLD_STR( ch->pcdata->homepage ) )
        fprintf( fp, "Homepage      %s~\n", ch->pcdata->homepage );
    if ( ch->pcdata->lair > 0 )
        fprintf( fp, "LAIR          %d\n", ch->pcdata->lair );
    if ( ch->pcdata->palace > 0 )
        fprintf( fp, "Palace          %d\n", ch->pcdata->palace );
    if ( VLD_STR( ch->pcdata->email ) )
        fprintf( fp, "Email         %s~\n", ch->pcdata->email );
    if ( ch->pcdata->icq > 0 )                         /* Samson 1-4-99 */
        fprintf( fp, "ICQ          %d\n", ch->pcdata->icq );
    if ( ch->pcdata->clanpoints > 0 )                  /* Samson 1-4-99 */
        fprintf( fp, "Clanpoints          %d\n", ch->pcdata->clanpoints );
    if ( ch->pcdata->tradelevel > 0 )
        fprintf( fp, "Tradelevel          %d\n", ch->pcdata->tradelevel );
    if ( ch->pcdata->tradeclass > 0 )
        fprintf( fp, "Tradeclass          %d\n", ch->pcdata->tradeclass );
    if ( ch->pcdata->craftpoints > 0 )
        fprintf( fp, "Craftpoints          %d\n", ch->pcdata->craftpoints );
    if ( ch->pcdata->banish > 0 )
        fprintf( fp, "Banish          %d\n", ch->pcdata->banish );
    if ( ch->pcdata->rprate > 0 )
        fprintf( fp, "Rprate          %d\n", ch->pcdata->rprate );
    if ( ch->pcdata->lastrated )
        fprintf( fp, "Lastrated       %ld\n", ch->pcdata->lastrated );
    if ( VLD_STR( ch->pcdata->bio ) )
        fprintf( fp, "Bio          %s~\n", strip_cr( ch->pcdata->bio ) );
    if ( VLD_STR( ch->pcdata->authed_by ) )
        fprintf( fp, "AuthedBy     %s~\n", ch->pcdata->authed_by );
    if ( ch->pcdata->min_snoop )
        fprintf( fp, "Minsnoop     %d\n", ch->pcdata->min_snoop );
    if ( VLD_STR( ch->pcdata->prompt ) )
        fprintf( fp, "Prompt       %s~\n", ch->pcdata->prompt );
    if ( VLD_STR( ch->pcdata->fprompt ) )
        fprintf( fp, "FPrompt       %s~\n", ch->pcdata->fprompt );
    if ( ch->pcdata->pagerlen != 24 )
        fprintf( fp, "Pagerlen     %d\n", ch->pcdata->pagerlen );
    if ( ch->pcdata->getsdoubleexp == TRUE )
        fprintf( fp, "GetsDExp     %d\n", 1 );
    else
        fprintf( fp, "GetsDExp     %d\n", 0 );
    if ( ch->pcdata->double_exp_timer != 0 )
        fprintf( fp, "DExpTimer    %d\n", ch->pcdata->double_exp_timer );
    if ( ch->pcdata->last_dexpupdate )
        fprintf( fp, "LDExpUpdate  %ld\n", ch->pcdata->last_dexpupdate );
    if ( ch->pcdata && ch->pcdata->preglory_hit )
        fprintf( fp, "Preglory_hit %d\n", ch->pcdata->preglory_hit );
    if ( ch->pcdata && ch->pcdata->preglory_mana )
        fprintf( fp, "Preglory_mana %d\n", ch->pcdata->preglory_mana );
    if ( ch->pcdata && ch->pcdata->preglory_move )
        fprintf( fp, "Preglory_move %d\n", ch->pcdata->preglory_move );
    if ( ch->pcdata && ch->pcdata->tmpheight )
        fprintf( fp, "tmpheight %d\n", ch->pcdata->tmpheight );
    if ( ch->pcdata && ch->pcdata->tmpcrawl )
        fprintf( fp, "tmpcrawl %d\n", ch->pcdata->tmpcrawl );
    if ( ch->pcdata && ch->pcdata->tmpweight )
        fprintf( fp, "tmpweight %d\n", ch->pcdata->tmpweight );
    if ( ch->pcdata && ch->pcdata->extlevel )
        fprintf( fp, "extlevel %d\n", ch->pcdata->extlevel );
    if ( ch->pcdata && ch->pcdata->tmplevel )
        fprintf( fp, "tmplevel %d\n", ch->pcdata->tmplevel );
    if ( ch->pcdata && ch->pcdata->tmpmax_hit )
        fprintf( fp, "tmpmax_hit %d\n", ch->pcdata->tmpmax_hit );
    if ( ch->pcdata && ch->pcdata->tmpmax_mana )
        fprintf( fp, "tmpmax_mana %d\n", ch->pcdata->tmpmax_mana );
    if ( ch->pcdata && ch->pcdata->tmpmax_move )
        fprintf( fp, "tmpmax_move %d\n", ch->pcdata->tmpmax_move );
    if ( ch->pcdata && ch->pcdata->tmprace )
        fprintf( fp, "tmprace %d\n", ch->pcdata->tmprace );
    if ( ch->pcdata && ch->pcdata->tmpclass )
        fprintf( fp, "tmpclass %d\n", ch->pcdata->tmpclass );
    if ( ch->pcdata && ch->pcdata->tmproom )
        fprintf( fp, "tmproom %d\n", ch->pcdata->tmproom );
    if ( ch->pcdata && ch->pcdata->timezone )
        fprintf( fp, "timezone %d\n", ch->pcdata->timezone );
    for ( pal = ch->pcdata->first_alias; pal; pal = pal->next ) {
        if ( !pal->name || !pal->cmd || !*pal->name || !*pal->cmd )
            continue;
        fprintf( fp, "Alias        %s~ %s~\n", pal->name, pal->cmd );
    }
    fprintf( fp, "Boards          %d", MAX_BOARD );
    for ( i = 0; i < MAX_BOARD; i++ )
        fprintf( fp, " %s %ld", boards[i].short_name, ch->pcdata->last2_note[i] );
    fprintf( fp, "\n" );

    if ( ch->pcdata->textspeed )
        fprintf( fp, "Textspeed          %d\n", ch->pcdata->textspeed );
    else
        fprintf( fp, "Textspeed          5\n" );

    fprintf( fp, "PotionTimer        %d %d %d\n", ch->pcdata->potionsoob, ch->pcdata->potionspve,
             ch->pcdata->potionspvp );
    fprintf( fp, "FirstExpR          %d\n", ch->firstexpratio );
    fprintf( fp, "SecondExpR         %d\n", ch->secondexpratio );
    fprintf( fp, "ThirdExpR          %d\n", ch->thirdexpratio );
    fprintf( fp, "FocusLevel         %d\n", ch->focus_level );  // Taon
    fprintf( fp, "UsedTrade          %d\n", ch->used_trade );   // Taon
    fprintf( fp, "Faith              %d\n", ch->faith );    // Taon
    fprintf( fp, "ChanInvite         %d\n", ch->chan_invite );  // Taon
    fprintf( fp, "QuestCurr          %d\n", ch->quest_curr );   // Taon
    fprintf( fp, "QuestAccum         %d\n", ch->quest_accum );  // Taon
    fprintf( fp, "MobCount           %d\n", ch->arena_mob_count );  // Taon
    fprintf( fp, "ObjCount           %d\n", ch->arena_obj_count );  // Taon
    fprintf( fp, "ArenaWins          %d\n", ch->arena_wins );   // Taon
    fprintf( fp, "ArenaLoss          %d\n", ch->arena_loss );   // Taon
    fprintf( fp, "MapToggle          %d\n", ch->map_toggle );   // Taon
    fprintf( fp, "MapSize            %d\n", ch->map_size ); // Taon
    fprintf( fp, "MapDescToggle      %d\n", ch->map_desc_toggle );  // Taon
    fprintf( fp, "MapNameToggle      %d\n", ch->map_name_toggle );  // Taon
    fprintf( fp, "MapType            %d\n", ch->map_type ); // Taon

    /*
     * If ch is ignoring players then store those players 
     */
    {
        IGNORE_DATA            *temp;

        for ( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
            fprintf( fp, "Ignored      %s~\n", temp->name );
    }

    {
        FOUND_AREA             *found;

        for ( found = ch->pcdata->first_area; found; found = found->next )
            if ( found && found->area_name )
                fprintf( fp, "FoundArea       %s~\n", found->area_name );
    }

    if ( IS_IMMORTAL( ch ) || WAS_IMMORTAL( ch ) ) {
        if ( VLD_STR( ch->pcdata->bamfin ) )
            fprintf( fp, "Bamfin       %s~\n", ch->pcdata->bamfin );
        if ( VLD_STR( ch->pcdata->bamfout ) )
            fprintf( fp, "Bamfout      %s~\n", ch->pcdata->bamfout );
        if ( ch->trust )
            fprintf( fp, "Trust        %d\n", ch->trust );
        if ( ch->pcdata && ch->pcdata->restore_time )
            fprintf( fp, "Restore_time %ld\n", ch->pcdata->restore_time );
        if ( ch->pcdata && ch->pcdata->heaven_time )
            fprintf( fp, "Heaven_time %ld\n", ch->pcdata->heaven_time );
        fprintf( fp, "WizInvis     %d\n", ch->pcdata->wizinvis );

        if ( ch->pcdata->alt1 )
            fprintf( fp, "Alt1       %s~\n", ch->pcdata->alt1 );
        if ( ch->pcdata->alt2 )
            fprintf( fp, "Alt2       %s~\n", ch->pcdata->alt2 );
        if ( ch->pcdata->alt3 )
            fprintf( fp, "Alt3       %s~\n", ch->pcdata->alt3 );
        if ( ch->pcdata->alt4 )
            fprintf( fp, "Alt4       %s~\n", ch->pcdata->alt4 );
        if ( ch->pcdata->alt5 )
            fprintf( fp, "Alt5       %s~\n", ch->pcdata->alt5 );
        if ( ch->pcdata->rand )
            fprintf( fp, "Rand       %s~\n", ch->pcdata->rand );
        if ( ch->pcdata->r_range_lo && ch->pcdata->r_range_hi )
            fprintf( fp, "RoomRange    %d %d\n", ch->pcdata->r_range_lo, ch->pcdata->r_range_hi );
        if ( ch->pcdata->o_range_lo && ch->pcdata->o_range_hi )
            fprintf( fp, "ObjRange     %d %d\n", ch->pcdata->o_range_lo, ch->pcdata->o_range_hi );
        if ( ch->pcdata->m_range_lo && ch->pcdata->m_range_hi )
            fprintf( fp, "MobRange     %d %d\n", ch->pcdata->m_range_lo, ch->pcdata->m_range_hi );
        if ( ch->pcdata && ch->pcdata->tmptrust )
            fprintf( fp, "tmptrust %d\n", ch->pcdata->tmptrust );
    }
    if ( ch->pcdata->council )
        fprintf( fp, "Council      %s~\n",
                 VLD_STR( ch->pcdata->council_name ) ? ch->pcdata->council_name : "" );
    if ( ch->pcdata->deity_name && ch->pcdata->deity_name[0] != '\0' )
        fprintf( fp, "Deity	     %s~\n", ch->pcdata->deity_name );
    if ( ch->pcdata->htown )
        fprintf( fp, "Htown      %s~\n",
                 VLD_STR( ch->pcdata->htown_name ) ? ch->pcdata->htown_name : "" );
    if ( ch->pcdata->city )
        fprintf( fp, "City      %s~\n",
                 VLD_STR( ch->pcdata->city_name ) ? ch->pcdata->city_name : "" );
    if ( ch->pcdata->clan_name && ch->pcdata->clan_name[0] != '\0' )
        fprintf( fp, "Clan         %s~\n", ch->pcdata->clan_name );
    if ( ch->pcdata->clan_timer > 0 )
        fprintf( fp, "ClanTimer    %d\n", ch->pcdata->clan_timer );
    fprintf( fp, "Flags        %d\n", ch->pcdata->flags );
    if ( ch->pcdata->release_date )
        fprintf( fp, "Helled       %d %s~\n", ( int ) ch->pcdata->release_date,
                 ch->pcdata->helled_by );
    fprintf( fp, "Bardtimer    %d\n", ch->pcdata->bard );
    fprintf( fp, "Bardsn       %d\n", ch->pcdata->bardsn );
    fprintf( fp, "PKills       %d\n", ch->pcdata->pkills );
    fprintf( fp, "PDeaths      %d\n", ch->pcdata->pdeaths );
    if ( get_timer( ch, TIMER_PKILLED ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) )
        fprintf( fp, "PTimer       %d\n", get_timer( ch, TIMER_PKILLED ) );
    fprintf( fp, "MKills       %d\n", ch->pcdata->mkills );
    fprintf( fp, "MDeaths      %d\n", ch->pcdata->mdeaths );
    fprintf( fp, "IllegalPK    %d\n", ch->pcdata->illegal_pk );
    fprintf( fp, "AttrPerm     %d %d %d %d %d %d %d\n", ch->perm_str, ch->perm_int, ch->perm_wis,
             ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );
    fprintf( fp, "AttrMod      %d %d %d %d %d %d %d\n", ch->mod_str, ch->mod_int, ch->mod_wis,
             ch->mod_dex, ch->mod_con, ch->mod_cha, ch->mod_lck );
    fprintf( fp, "Condition    %d %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1],
             ch->pcdata->condition[2], ch->pcdata->condition[3] );
    if ( ch->desc && ch->desc->host )
        fprintf( fp, "Site         %s\n", ch->desc->host );
    else
        fprintf( fp, "Site         (Link-Dead)\n" );
    fprintf( fp, "LastNews     %ld\n", ch->pcdata->last_read_news );
    fprintf( fp, "Holdbreath %d\n", ch->pcdata->holdbreath );
    fprintf( fp, "Frostbite %d\n", ch->pcdata->frostbite );
    fprintf( fp, "Adjust %d\n", ch->pcdata->adjust );
    for ( sn = 1; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            continue;

        if ( ch->pcdata->learned[sn] > 0 || ch->pcdata->dlearned[sn] > 0 ) {
            switch ( skill_table[sn]->type ) {
                default:
                    fprintf( fp, "NNSkill       %d %d '%s' %d %d\n", ch->pcdata->learned[sn],
                             ch->pcdata->dlearned[sn], skill_table[sn]->name,
                             ch->pcdata->dshowlearned[sn], ch->pcdata->dshowdlearned[sn] );
                    break;
                case SKILL_SPELL:
                    fprintf( fp, "NNSpell       %d %d '%s' %d %d\n", ch->pcdata->learned[sn],
                             ch->pcdata->dlearned[sn], skill_table[sn]->name,
                             ch->pcdata->dshowlearned[sn], ch->pcdata->dshowdlearned[sn] );
                    break;
                case SKILL_WEAPON:
                    fprintf( fp, "NNWeapon      %d %d '%s' %d %d\n", ch->pcdata->learned[sn],
                             ch->pcdata->dlearned[sn], skill_table[sn]->name,
                             ch->pcdata->dshowlearned[sn], ch->pcdata->dshowdlearned[sn] );
                    break;
                case SKILL_TONGUE:
                    fprintf( fp, "NNTongue      %d %d '%s' %d %d\n", ch->pcdata->learned[sn],
                             ch->pcdata->dlearned[sn], skill_table[sn]->name,
                             ch->pcdata->dshowlearned[sn], ch->pcdata->dshowdlearned[sn] );
                    break;
                case SKILL_SONG:
                    fprintf( fp, "NNSong        %d %d '%s' %d %d\n", ch->pcdata->learned[sn],
                             ch->pcdata->dlearned[sn], skill_table[sn]->name,
                             ch->pcdata->dshowlearned[sn], ch->pcdata->dshowdlearned[sn] );
                    break;
            }

        }
    }

    /*
     * Volk: AffectData and Affect have L on the end now, because I want spell levels to
     * save 
     */
    for ( paf = ch->first_affect; paf; paf = paf->next ) {
        if ( paf->type >= 0 && ( skill = get_skilltype( paf->type ) ) == NULL )
            continue;
        if ( paf->type >= 0 && paf->type < TYPE_HIT )
            fprintf( fp, "AffectDataL   '%s' %3d %3d %3d %3d %s\n", skill->name, paf->level,
                     paf->duration, paf->modifier, paf->location,
                     print_bitvector( &paf->bitvector ) );
        else
            fprintf( fp, "AffectL       %3d %3d %3d %3d %3d %s\n", paf->type, paf->level,
                     paf->duration, paf->modifier, paf->location,
                     print_bitvector( &paf->bitvector ) );
    }

    // track = URANGE(2, ((ch->level + 3) * MAX_KILLTRACK) / LEVEL_DEMIGOD,
    // MAX_KILLTRACK);
    for ( sn = 0; sn < MAX_KILLTRACK; sn++ ) {
        if ( ch->pcdata->killed[sn].vnum == 0 )
            break;
        fprintf( fp, "Killed       %d\n", ch->pcdata->killed[sn].vnum );
    }

    {
        PKILLED_DATA           *pkill;

        /*
         * Lets go ahead and make it update the killed list (Will remove ones not needed) 
         */
        has_pkilled( ch, ( char * ) "Thoric" );

        for ( pkill = ch->pcdata->first_pkill; pkill; pkill = pkill->next )
            fprintf( fp, "PKilled      %ld %s~\n", pkill->timekilled, pkill->name );
    }

    {
        CHQUEST_DATA           *chquest;

        for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
            fprintf( fp, "THYQuestN %d %d %d %d %d %d\n", chquest->questnum, chquest->progress,
                     chquest->questlimit, chquest->chaplimit, chquest->times, chquest->kamount );
        }
    }

    /*
     * Save color values - Samson 9-29-98 
     */
    {
        int                     x;

        fprintf( fp, "MaxColors    %d\n", MAX_COLORS );
        fprintf( fp, "Colors       " );
        for ( x = 0; x < MAX_COLORS; x++ )
            fprintf( fp, "%d ", ch->colors[x] );
        fprintf( fp, "\n" );
    }
#ifdef I3
    i3_save_char( ch, fp );
#endif

    if ( VLD_STR( ch->pcdata->vamp_status ) )
        fprintf( fp, "Vamp_status         %s~\n", ch->pcdata->vamp_status );
/*
  if(VLD_STR(ch->pcdata->trade))
    fprintf(fp, "Trade         %s~\n", ch->pcdata->trade);
*/
    fprintf( fp, "End\n\n" );
    return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE * fp, int iNest, short os_type, bool copyover )
{
    EXTRA_DESCR_DATA       *ed;
    AFFECT_DATA            *paf;
    short                   wear,
                            wear_loc,
                            x;
    bool                    pkdisarmed = FALSE;

    if ( iNest >= MAX_NEST ) {
        bug( "fwrite_obj: iNest hit MAX_NEST %d", iNest );
        return;
    }

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->prev_content && os_type != OS_CORPSE ) {
        if ( os_type == OS_CARRY )
            fwrite_obj( ch, obj->prev_content, fp, iNest, OS_CARRY, copyover );
    }

    /*
     * Castrate storage characters.
     * Catch deleted objects                                    -Thoric
     * Do NOT save prototype items!     -Thoric
     */
    if ( !copyover ) {
        if ( ( obj->item_type == ITEM_KEY && !IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) )
             || obj_extracted( obj ) || IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
            return;
    }

    /*
     * Munch the ITEM_PKDISARMED flag, if it is set, as it should NOT be saved.
     */
    if ( IS_OBJ_STAT( obj, ITEM_PKDISARMED ) ) {
        xREMOVE_BIT( obj->extra_flags, ITEM_PKDISARMED );
        if ( obj->item_type == ITEM_WEAPON )
            pkdisarmed = TRUE;
    }

    /*
     * DO NOT save corpses lying on the ground as a copyover item, they already saved 
     * elsewhere! - Samson
     */
    if ( copyover && obj->item_type == ITEM_CORPSE_PC )
        return;

    /*
     * Corpse saving. -- Altrag 
     */
    fprintf( fp,
             ( os_type == OS_CORPSE ? "#CORPSE\n" : os_type ==
               OS_VAULT ? "#VAULT\n" : "#OBJECT\n" ) );

    if ( iNest )
        fprintf( fp, "Nest         %d\n", iNest );
    if ( obj->count > 1 )
        fprintf( fp, "Count        %d\n", obj->count );
    if ( obj->owner )
        fprintf( fp, "Owner        %s~\n", obj->owner );
    if ( VLD_STR( obj->name ) && VLD_STR( obj->pIndexData->name )
         && str_cmp( obj->name, obj->pIndexData->name ) )
        fprintf( fp, "Name         %s~\n", obj->name );
    if ( VLD_STR( obj->short_descr ) && VLD_STR( obj->short_descr )
         && str_cmp( obj->short_descr, obj->pIndexData->short_descr ) )
        fprintf( fp, "ShortDescr   %s~\n", obj->short_descr );
    if ( VLD_STR( obj->description ) && VLD_STR( obj->pIndexData->description )
         && str_cmp( obj->description, obj->pIndexData->description ) )
        fprintf( fp, "Description  %s~\n", obj->description );
    if ( VLD_STR( obj->action_desc ) && VLD_STR( obj->pIndexData->action_desc )
         && str_cmp( obj->action_desc, obj->pIndexData->action_desc ) )
        fprintf( fp, "ActionDesc   %s~\n", obj->action_desc );
    fprintf( fp, "Vnum         %d\n", obj->pIndexData->vnum );
    if ( ( os_type == OS_CORPSE || copyover ) && obj->in_room ) {
        fprintf( fp, "Room         %d\n", obj->in_room->vnum );
        fprintf( fp, "Rvnum     %d\n", obj->room_vnum );
    }
    if ( !xSAME_BITS( obj->extra_flags, obj->pIndexData->extra_flags ) )
        fprintf( fp, "ExtraFlags   %s\n", print_bitvector( &obj->extra_flags ) );
    if ( obj->wear_flags != obj->pIndexData->wear_flags )
        fprintf( fp, "WearFlags    %d\n", obj->wear_flags );
    wear_loc = -1;
    for ( wear = 0; wear < MAX_WEAR; wear++ )
        for ( x = 0; x < MAX_LAYERS; x++ )
            if ( obj == save_equipment[wear][x] ) {
                wear_loc = wear;
                break;
            }
            else if ( !save_equipment[wear][x] )
                break;
    if ( wear_loc != -1 )
        fprintf( fp, "WearLoc      %d\n", wear_loc );
    if ( obj->item_type != obj->pIndexData->item_type )
        fprintf( fp, "ItemType     %d\n", obj->item_type );
    if ( obj->weight != obj->pIndexData->weight )
        fprintf( fp, "Weight       %d\n", obj->weight );
    if ( obj->level )
        fprintf( fp, "Level        %d\n", obj->level );
    if ( obj->glory > 0 )
        fprintf( fp, "Glory        %d\n", obj->glory );
    if ( obj->timer )
        fprintf( fp, "Timer        %d\n", obj->timer );
    if ( obj->cost != obj->pIndexData->cost )
        fprintf( fp, "Cost         %d\n", obj->cost );
/*    Thought might fix the problem with object currtypes, apparantly not yet */
    if ( obj->currtype )
        fprintf( fp, "Currtype     %d\n", obj->currtype );
    if ( obj->color )
        fprintf( fp, "Color        %d\n", obj->color );

    fprintf( fp, "Size   %d\n", obj->size );

    if ( obj->value[0] || obj->value[1] || obj->value[2] || obj->value[3] || obj->value[4]
         || obj->value[5] || obj->value[6] )
        fprintf( fp, "Values       %d %d %d %d %d %d %d\n", obj->value[0], obj->value[1],
                 obj->value[2], obj->value[3], obj->value[4], obj->value[5], obj->value[6] );

    switch ( obj->item_type ) {
        case ITEM_PILL:                               /* was down there with staff and * 
                                                        * wand, wrongly - Scryn */
        case ITEM_POTION:
        case ITEM_SCROLL:
            if ( IS_VALID_SN( obj->value[1] ) )
                fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]]->name );

            if ( IS_VALID_SN( obj->value[2] ) )
                fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]]->name );

            if ( IS_VALID_SN( obj->value[3] ) )
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );

            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            if ( IS_VALID_SN( obj->value[3] ) )
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );

            break;
        case ITEM_SALVE:
            if ( IS_VALID_SN( obj->value[4] ) )
                fprintf( fp, "Spell 4      '%s'\n", skill_table[obj->value[4]]->name );

            if ( IS_VALID_SN( obj->value[6] ) )
                fprintf( fp, "Spell 6      '%s'\n", skill_table[obj->value[6]]->name );
            break;
    }

    for ( paf = obj->first_affect; paf; paf = paf->next ) {
        /*
         * Save extra object affects     -Thoric
         */

        if ( paf->type < 0 || paf->type >= top_sn ) {
            fprintf( fp, "Affect       %d %d %d %d %s\n",
                     paf->type,
                     paf->duration,
                     ( ( paf->location == APPLY_WEAPONSPELL
                         || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_REMOVESPELL
                         || paf->location == APPLY_STRIPSN
                         || paf->location == APPLY_RECURRINGSPELL )
                       && IS_VALID_SN( paf->modifier ) ) ? skill_table[paf->modifier]->slot : paf->
                     modifier, paf->location, print_bitvector( &paf->bitvector ) );
        }
        else
            fprintf( fp, "AffectData   '%s' %d %d %d %s\n",
                     skill_table[paf->type]->name,
                     paf->duration,
                     ( ( paf->location == APPLY_WEAPONSPELL
                         || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_REMOVESPELL
                         || paf->location == APPLY_STRIPSN
                         || paf->location == APPLY_RECURRINGSPELL )
                       && IS_VALID_SN( paf->modifier ) ) ? skill_table[paf->modifier]->slot : paf->
                     modifier, paf->location, print_bitvector( &paf->bitvector ) );
    }

    for ( ed = obj->first_extradesc; ed; ed = ed->next )
        fprintf( fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description );

    fprintf( fp, "End\n\n" );

    if ( obj->first_content )
        fwrite_obj( ch, obj->last_content, fp, iNest + 1, OS_CARRY, copyover );

    /*
     * Re-add the flag, if it should be there.
     */
    if ( pkdisarmed == TRUE )
        xSET_BIT( obj->extra_flags, ITEM_PKDISARMED );

    return;
}

/* Load a char and inventory into a new ch structure. */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name, bool preload, bool copyover, bool quiet )
{
    char                    strsave[MIL];
    CHAR_DATA              *ch;
    FILE                   *fp;
    bool                    found;
    struct stat             fst;
    int                     i,
                            x;
    char                    buf[MIL];

    CREATE( ch, CHAR_DATA, 1 );

    for ( x = 0; x < MAX_WEAR; x++ )
        for ( i = 0; i < MAX_LAYERS; i++ )
            save_equipment[x][i] = NULL;
    clear_char( ch );
    loading_char = ch;
    CREATE( ch->pcdata, PC_DATA, 1 );

    d->character = ch;
    ch->desc = d;
    ch->pcdata->filename = STRALLOC( name );
    ch->name = NULL;
    ch->act = multimeb( PLR_BLANK, PLR_COMBINE, PLR_PROMPT, -1 );
    ch->perm_str = 13;
    ch->perm_int = 13;
    ch->perm_wis = 13;
    ch->perm_dex = 13;
    ch->perm_con = 13;
    ch->perm_cha = 13;
    ch->perm_lck = 13;
    ch->hate_level = 0;                                /* Default for hate_level */
    ch->ward_dam = 0;
    ch->degree = 1;
    ch->kinetic_dam = 0;
    ch->no_resistant = 0;
    ch->no_susceptible = 0;
    ch->no_immune = 0;
    ch->was_in_room = NULL;
    xCLEAR_BITS( ch->no_affected_by );

    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->blood = 10;
    ch->pcdata->nuisance = NULL;
    ch->pcdata->wizinvis = 0;
    ch->pcdata->charmies = 0;
    ch->pcdata->first_area = ch->pcdata->last_area = NULL;
    ch->pcdata->tmptrust = 0;
    ch->pcdata->extlevel = 0;
    ch->pcdata->tmplevel = 0;
    ch->pcdata->tmpmax_mana = 0;
    ch->pcdata->tmpmax_move = 0;
    ch->pcdata->tmpmax_hit = 0;
    ch->pcdata->preglory_hit = 0;
    ch->pcdata->preglory_mana = 0;
    ch->pcdata->preglory_move = 0;
    ch->pcdata->tmpheight = 0;
    ch->pcdata->tmpcrawl = 0;
    ch->pcdata->tmpweight = 0;
    ch->pcdata->tmpclass = 0;
    ch->pcdata->tmprace = 0;
    ch->pcdata->tmproom = 0;
    if ( ch->pcdata->timezone )
        ch->pcdata->timezone = 0;
    ch->mental_state = -10;
    ch->mobinvis = 0;
    for ( i = 0; i < MAX_SKILL; i++ ) {
        ch->pcdata->learned[i] = 0;
        ch->pcdata->dlearned[i] = 0;
        ch->pcdata->dshowlearned[i] = FALSE;
        ch->pcdata->dshowdlearned[i] = FALSE;
    }
    ch->pcdata->release_date = 0;
    ch->pcdata->helled_by = NULL;
    ch->pcdata->apply_blood = 0;
    ch->saving_poison_death = 0;
    ch->saving_wand = 0;
    ch->saving_para_petri = 0;
    ch->saving_breath = 0;
    ch->saving_spell_staff = 0;
    ch->pcdata->deity_name = STRALLOC( "" );
    ch->pcdata->deity = NULL;
    ch->style = STYLE_FIGHTING;
    ch->pcdata->pagerlen = 24;
    ch->pcdata->first_ignored = NULL;                  /* Ignore list */
    ch->pcdata->last_ignored = NULL;
    ch->pcdata->clan = NULL;
    ch->morph = NULL;
    ch->pcdata->afkbuf = NULL;                         /* Initialize AFK reason buffer -
                                                        * Samson 9-2-98 */
    ch->pcdata->email = NULL;                          /* Initialize email address -
                                                        * Samson 1-4-99 */
    ch->pcdata->homepage = NULL;                       /* Initialize homepage - Samson
                                                        * 1-4-99 */
    ch->pcdata->icq = 0;                               /* Initalize icq# - Samson 1-4-99 */
    ch->pcdata->adjust = 0;
    ch->pcdata->clanpoints = 0;
    ch->pcdata->tradelevel = 0;
    ch->pcdata->tradeclass = 0;
    ch->pcdata->craftpoints = 0;
    ch->pcdata->banish = 0;
    ch->pcdata->rprate = 0;
    ch->pcdata->lair = 0;
    ch->pcdata->palace = 0;
    ch->pcdata->copyover = FALSE;                      /* Never changed except when PC is 
                                                        * saved during copyover save */
    ch->pcdata->vamp_status = NULL;
    ch->pcdata->board = &boards[DEFAULT_BOARD];
    ch->pcdata->boards = 0;
    ch->pcdata->tag_flags = 0;
    ch->pcdata->bank = NULL;
#ifdef I3
    i3_init_char( ch );
#endif

    found = FALSE;
    snprintf( strsave, MIL, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
    if ( stat( strsave, &fst ) != -1 ) {
        if ( d->connected != CON_PLOADED && !quiet ) {
            if ( preload )
                snprintf( buf, MIL, "Preloading player data for: %s", ch->pcdata->filename );
            else
                snprintf( buf, MIL, "Loading player data for %s (%dK)", ch->pcdata->filename,
                          ( int ) fst.st_size / 1024 );
            log_string_plus( buf, LOG_COMM, LEVEL_AJ_CPL );
        }
    }
    else {                                             /* No player file */

        ch->name = STRALLOC( name );
        ch->short_descr = STRALLOC( "" );
        ch->long_descr = STRALLOC( "" );
        ch->description = STRALLOC( "" );
        ch->editor = NULL;
        ch->pcdata->clan_name = STRALLOC( "" );
        ch->pcdata->clan = NULL;
        ch->pcdata->council_name = STRALLOC( "" );
        ch->pcdata->council = NULL;
        ch->pcdata->deity_name = STRALLOC( "" );
        ch->pcdata->deity = NULL;
        ch->pcdata->pet = NULL;
        ch->pcdata->pwd = STRALLOC( "" );
        ch->pcdata->bamfin = STRALLOC( "" );
        ch->pcdata->bamfout = STRALLOC( "" );
        ch->pcdata->rank = STRALLOC( "" );
        ch->pcdata->bestowments = STRALLOC( "" );
        ch->pcdata->title = STRALLOC( "" );
        ch->pcdata->homepage = STRALLOC( "" );
        ch->pcdata->bio = STRALLOC( "" );
        ch->pcdata->authed_by = STRALLOC( "" );
        ch->pcdata->prompt = STRALLOC( "" );
        ch->pcdata->fprompt = STRALLOC( "" );
        ch->pcdata->r_range_lo = 0;
        ch->pcdata->r_range_hi = 0;
        ch->pcdata->m_range_lo = 0;
        ch->pcdata->m_range_hi = 0;
        ch->pcdata->o_range_lo = 0;
        ch->pcdata->o_range_hi = 0;
        ch->pcdata->wizinvis = 0;
        update_aris( ch );
        loading_char = NULL;
        return found;
    }

    if ( ( fp = FileOpen( strsave, "r" ) ) != NULL ) {
        int                     iNest;

        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;
        found = TRUE;
        /*
         * Cheat so that bug will show line #'s -- Altrag 
         */
        fpArea = fp;
        mudstrlcpy( strArea, strsave, MIL );
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }
            if ( letter != '#' ) {
                bug( "Load_char_obj: # not found. [%s]", name );
                break;
            }
            word = fread_word( fp );
            if ( !strcmp( word, "PLAYER" ) ) {
                fread_char( ch, fp, preload, copyover );
                if ( preload )
                    break;
            }
            else if ( !str_cmp( word, "OBJECT" ) )     /* Objects */
                fread_obj( ch, fp, OS_CARRY );
            else if ( !str_cmp( word, "MorphData" ) )  /* Morphs */
                fread_morph_data( ch, fp );
            else if ( !strcmp( word, "COMMENT" ) )
                fread_comment( ch, fp );               /* Comments */
            else if ( !strcmp( word, "MOBILE" ) ) {
                CHAR_DATA              *mob;

                if ( ( mob = fread_mobile( fp ) ) ) {
                    ch->pcdata->pet = mob;
                    mob->master = ch;
                    xSET_BIT( mob->affected_by, AFF_CHARM );
                }
                else
                    bug( "load_char_obj: Deleted mob saved on %s - skipping", ch->name );
            }
            else if ( !str_cmp( word, "VARIABLE" ) )   // Quest Flags
                fread_variable( ch, fp );

            else if ( !strcmp( word, "END" ) )         /* Done */
                break;
            else {
                bug( "Load_char_obj: bad section. [%s]", name );
                break;
            }
        }
        FileClose( fp );
        fpArea = NULL;
        mudstrlcpy( strArea, "$", MIL );
    }

    if ( !ch->name )
        ch->name = STRALLOC( name );
    if ( !ch->pcdata->clan_name ) {
        ch->pcdata->clan_name = STRALLOC( "" );
        ch->pcdata->clan = NULL;
    }
    if ( !ch->pcdata->council_name ) {
        ch->pcdata->council_name = STRALLOC( "" );
        ch->pcdata->council = NULL;
    }
    if ( !ch->pcdata->deity_name ) {
        ch->pcdata->deity_name = STRALLOC( "" );
        ch->pcdata->deity = NULL;
    }
    if ( !ch->pcdata->bio )
        ch->pcdata->bio = STRALLOC( "" );

    if ( !ch->pcdata->authed_by )
        ch->pcdata->authed_by = STRALLOC( "" );

    if ( IS_IMMORTAL( ch ) ) {
        if ( ch->pcdata->wizinvis < 2 )
            ch->pcdata->wizinvis = ch->level;
        assign_area( ch, TRUE );
    }
    if ( xIS_SET( ch->act, PLR_FLEE ) )
        xREMOVE_BIT( ch->act, PLR_FLEE );
    if ( file_ver > 1 ) {
        for ( i = 0; i < MAX_WEAR; i++ ) {
            for ( x = 0; x < MAX_LAYERS; x++ ) {
                if ( save_equipment[i][x] ) {
                    equip_char( ch, save_equipment[i][x], i );
                    save_equipment[i][x] = NULL;
                }
                else
                    break;
            }
        }
    }

    /*
     * Rebuild affected_by and RIS to catch errors - FB 
     */
    update_aris( ch );
    loading_char = NULL;
    return found;
}

/* Read in a char. */
void fread_char( CHAR_DATA *ch, FILE * fp, bool preload, bool copyover )
{
    char                    buf[MSL];
    char                   *line;
    const char             *word;
    int                     x1,
                            x2,
                            x3,
                            x4,
                            x5,
                            x6,
                            x7;
    short                   killcnt;
    bool                    fMatch;
    int                     max_colors = 0;            /* Color code */
    BANK_DATA              *bank = NULL;
    int                     tmpi[MAX_CURR_TYPE];
    int                     currtime = time( 0 );

    for ( x1 = 0; x1 < MAX_CURR_TYPE; x1++ )
        tmpi[x1] = 0;

    file_ver = 0;
    killcnt = 0;
    /*
     * Setup color values in case player has none set - Samson 
     */
    memcpy( &ch->colors, &default_set, sizeof( default_set ) );
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        if ( !str_cmp( word, "ÿ" ) )
            word = "End";
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "Act", ch->act, fread_bitvector( fp ) );
                KEY( "AffectedBy", ch->affected_by, fread_bitvector( fp ) );
                if ( file_ver < 5 )
                    find_old_age( ch );
                else {
                    if ( !str_cmp( word, "Age" ) ) {
                        line = fread_line( fp );
                        x1 = x2 = x3 = x4 = 0;
                        sscanf( line, "%d %d %d %d", &x1, &x2, &x3, &x4 );
                        ch->pcdata->age_bonus = x1;
                        ch->pcdata->day = x2;
                        ch->pcdata->month = x3;
                        ch->pcdata->year = x4;
                        fMatch = TRUE;
                        break;
                    }
                }
                KEY( "Alignment", ch->alignment, fread_number( fp ) );
                KEY( "AppBlood", ch->pcdata->apply_blood, fread_number( fp ) );
                KEY( "ArenaWins", ch->arena_wins, fread_number( fp ) );
                KEY( "ArenaLoss", ch->arena_loss, fread_number( fp ) );
                KEY( "Armor", ch->armor, fread_number( fp ) );
                if ( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) ) {
                    AFFECT_DATA            *paf;

                    if ( preload ) {
                        fMatch = TRUE;
                        fread_to_eol( fp );
                        break;
                    }
                    CREATE( paf, AFFECT_DATA, 1 );

                    if ( !strcmp( word, "Affect" ) )
                        paf->type = fread_number( fp );
                    else {
                        int                     sn;
                        char                   *sname = fread_word( fp );

                        if ( ( sn = skill_lookup( sname ) ) < 0 ) {
                            if ( ( sn = herb_lookup( sname ) ) < 0 )
                                bug( "%s: unknown skill (%s).", __FUNCTION__, sname );
                        }
                        paf->type = sn;
                    }
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    if ( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
                         || paf->location == APPLY_RECURRINGSPELL )
                        paf->modifier = slot_lookup( paf->modifier );
                    paf->bitvector = fread_bitvector( fp );
                    LINK( paf, ch->first_affect, ch->last_affect, next, prev );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "AffectL" ) || !strcmp( word, "AffectDataL" ) ) {
                    AFFECT_DATA            *paf;
                    bool                    dadd = FALSE;

                    if ( preload ) {
                        fMatch = TRUE;
                        fread_to_eol( fp );
                        break;
                    }
                    CREATE( paf, AFFECT_DATA, 1 );

                    if ( !strcmp( word, "Affect" ) )
                        paf->type = fread_number( fp );
                    else {
                        int                     sn;
                        char                   *sname = fread_word( fp );

                        if ( ( sn = skill_lookup( sname ) ) < 0 ) {
                            if ( ( sn = herb_lookup( sname ) ) < 0 ) {
                                bug( "%s: unknown skill (%s).", __FUNCTION__, sname );
                                dadd = TRUE;
                            }
                        }
                        paf->type = sn;
                    }
                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    if ( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
                         || paf->location == APPLY_RECURRINGSPELL )
                        paf->modifier = slot_lookup( paf->modifier );
                    paf->bitvector = fread_bitvector( fp );
                    if ( dadd )
                        DISPOSE( paf );
                    else
                        LINK( paf, ch->first_affect, ch->last_affect, next, prev );
                    fMatch = TRUE;
                    break;
                }
                if ( !str_cmp( word, "Alias" ) ) {

                    if ( preload ) {
                        fMatch = TRUE;
                        fread_to_eol( fp );
                        break;
                    }
                    CREATE( pal, ALIAS_DATA, 1 );
                    pal->name = fread_string( fp );
                    pal->cmd = fread_string( fp );
                    LINK( pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Alt1" ) ) {
                    ch->pcdata->alt1 = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Alt2" ) ) {
                    ch->pcdata->alt2 = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Alt3" ) ) {
                    ch->pcdata->alt3 = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Alt4" ) ) {
                    ch->pcdata->alt4 = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Alt5" ) ) {
                    ch->pcdata->alt5 = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Rand" ) ) {
                    ch->pcdata->rand = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "AttrMod" ) ) {
                    line = fread_line( fp );
                    x1 = x2 = x3 = x4 = x5 = x6 = x7 = 13;
                    sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
                    ch->mod_str = x1;
                    ch->mod_int = x2;
                    ch->mod_wis = x3;
                    ch->mod_dex = x4;
                    ch->mod_con = x5;
                    ch->mod_cha = x6;
                    ch->mod_lck = x7;
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "AttrPerm" ) ) {
                    line = fread_line( fp );
                    x1 = x2 = x3 = x4 = x5 = x6 = x7 = 13;
                    sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
                    ch->perm_str = x1;
                    ch->perm_int = x2;
                    ch->perm_wis = x3;
                    ch->perm_dex = x4;
                    ch->perm_con = x5;
                    ch->perm_cha = x6;
                    ch->perm_lck = x7;
                    fMatch = TRUE;
                    break;
                }
                KEY( "AuthedBy", ch->pcdata->authed_by, fread_string( fp ) );
                KEY( "Adjust", ch->pcdata->adjust, fread_number( fp ) );
                break;
            case 'B':
                KEY( "Balance", x1, fread_number( fp ) );   /* Just stopping the bug
                                                             * messages */
                KEY( "Banish", ch->pcdata->banish, fread_number( fp ) );
                KEY( "Bardtimer", ch->pcdata->bard, fread_number( fp ) );
                KEY( "Bardsn", ch->pcdata->bardsn, fread_number( fp ) );
                KEY( "Bamfin", ch->pcdata->bamfin, fread_string( fp ) );
                KEY( "Bamfout", ch->pcdata->bamfout, fread_string( fp ) );
                if ( !str_cmp( word, "BankMoney" ) ) {
                    x1 = x2 = 0;
                    while ( ( x1 = fread_number( fp ) ) >= 0 && x2 < MAX_CURR_TYPE )
                        tmpi[x2++] = x1;
                    fread_to_eol( fp );
                    fMatch = TRUE;
                    if ( !preload && !( bank = find_bank( ch->name ) )
                         && ( tmpi[CURR_GOLD] > 0 || tmpi[CURR_SILVER] > 0 || tmpi[CURR_COPPER] > 0
                              || tmpi[CURR_BRONZE] > 0 ) ) {
                        CREATE( bank, BANK_DATA, 1 );
                        bank->name = STRALLOC( ch->name );
                        bank->password = NULL;
                        bank->bronze = 0;
                        bank->copper = 0;
                        bank->gold = 0;
                        bank->silver = 0;
                        for ( x1 = 0; x1 < MAX_CURR_TYPE; x1++ ) {
                            if ( x1 == CURR_GOLD )
                                bank->gold += tmpi[x1];
                            else if ( x1 == CURR_SILVER )
                                bank->silver += tmpi[x1];
                            else if ( x1 == CURR_COPPER )
                                bank->copper += tmpi[x1];
                            else if ( x1 == CURR_BRONZE )
                                bank->bronze += tmpi[x1];
                        }
                        ch->pcdata->lastaccountcreated = currtime;
                        ch->pcdata->bank = bank;
                    }
                    break;
                }
                KEY( "BaseHp", ch->temp_base_hit, fread_number( fp ) );
                if ( !str_cmp( word, "Boards" ) ) {
                    int                     i,
                                            num = fread_number( fp );   /* number of
                                                                         * boards saved */
                    char                   *boardname;

                    for ( ; num; num-- ) {             /* for each of the board saved */
                        boardname = fread_word( fp );
                        i = board_lookup( boardname ); /* find board number */
                        if ( i == BOARD_NOTFOUND ) {   /* Does board still exist ? */
                            snprintf( buf, MSL,
                                      "fread_char: %s had unknown board name: %s. Skipped.",
                                      ch->name, boardname );
                            log_string( buf );
                            fread_number( fp );        /* read last_note and skip info */
                        }
                        else                           /* Save it */
                            ch->pcdata->last2_note[i] = fread_number( fp );
                    }                                  /* for */
                    fMatch = TRUE;
                }                                      /* Boards */
                KEY( "Bestowments", ch->pcdata->bestowments, fread_string( fp ) );
                if ( !strcmp( word, "Blood" ) ) {
                    ch->blood = fread_number( fp );
                    ch->max_blood = fread_number( fp );
                    if ( ch->max_blood < 20 && ch->max_mana > 80 )
                        ch->max_blood = ch->max_mana / 4;
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Bank" ) ) {
                    char                   *bname = fread_flagstring( fp );
                    BANK_DATA              *bank = find_bank( bname );

                    if ( !bank ) {
                        if ( !preload )
                            ch_printf( ch, "Account %s no longer exist.\r\n", bname );
                    }
                    else
                        ch->pcdata->bank = bank;

                    fMatch = TRUE;
                    break;
                }
                KEY( "Bio", ch->pcdata->bio, fread_string( fp ) );
                break;
            case 'C':
                KEY( "Channels", ch->pcdata->chan_listen, fread_string( fp ) );
                if ( !strcmp( word, "Clan" ) ) {
                    ch->pcdata->clan_name = fread_string( fp );
                    if ( !preload && VLD_STR( ch->pcdata->clan_name )
                         && ( ch->pcdata->clan = get_clan( ch->pcdata->clan_name ) ) == NULL ) {
                        ch_printf( ch,
                                   "Warning: the organization %s no longer exists, and therefore you no longer\r\nbelong to that organization.\r\n",
                                   ch->pcdata->clan_name );
                        if ( VLD_STR( ch->pcdata->clan_name ) )
                            STRFREE( ch->pcdata->clan_name );
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "ChanInvite", ch->chan_invite, fread_number( fp ) );
                if ( !strcmp( word, "City" ) ) {
                    ch->pcdata->city_name = fread_string( fp );
                    if ( !preload && VLD_STR( ch->pcdata->city_name )
                         && ( ch->pcdata->city = get_city( ch->pcdata->city_name ) ) == NULL ) {
                        ch_printf( ch,
                                   "Warning: the city %s no longer exists, and herefore you no longer\r\nbelong to a city.\r\n",
                                   ch->pcdata->city_name );
                        if ( VLD_STR( ch->pcdata->city_name ) )
                            STRFREE( ch->pcdata->city_name );
                    }
                    fMatch = TRUE;
                    break;
                }

                KEY( "Clanpoints", ch->pcdata->clanpoints, fread_number( fp ) );
                KEY( "ClanTimer", ch->pcdata->clan_timer, fread_number( fp ) );
                KEY( "Class", ch->Class, fread_number( fp ) );
                /*
                 * Load color values - Samson 9-29-98 
                 */
                {
                    int                     x;

                    if ( !str_cmp( word, "Colors" ) ) {
                        for ( x = 0; x < max_colors; x++ )
                            ch->colors[x] = fread_number( fp );
                        fMatch = TRUE;
                        break;
                    }
                }
                if ( !str_cmp( word, "Condition" ) ) {
                    line = fread_line( fp );
                    sscanf( line, "%d %d %d %d", &x1, &x2, &x3, &x4 );
                    ch->pcdata->condition[0] = x1;
                    ch->pcdata->condition[1] = x2;
                    ch->pcdata->condition[2] = x3;
                    ch->pcdata->condition[3] = x4;
                    fMatch = TRUE;
                    break;
                }
                KEY( "Craftpoints", ch->pcdata->craftpoints, fread_number( fp ) );
                if ( !strcmp( word, "Council" ) ) {
                    ch->pcdata->council_name = fread_string( fp );
                    if ( !preload && VLD_STR( ch->pcdata->council_name )
                         && ( ch->pcdata->council =
                              get_council( ch->pcdata->council_name ) ) == NULL ) {
                        ch_printf( ch,
                                   "Warning: the council %s no longer exists, and herefore you no longer\r\nbelong to a council.\r\n",
                                   ch->pcdata->council_name );
                        if ( VLD_STR( ch->pcdata->council_name ) )
                            STRFREE( ch->pcdata->council_name );
                    }
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'D':
                KEY( "DExpTimer", ch->pcdata->double_exp_timer, fread_number( fp ) );
                KEY( "Damroll", ch->damroll, fread_number( fp ) );
                KEY( "Deaf", ch->deaf, fread_bitvector( fp ) );
                if ( !strcmp( word, "Deity" ) ) {
                    ch->pcdata->deity_name = fread_string( fp );

                    if ( !preload && ch->pcdata->deity_name[0] != '\0'
                         && ( ch->pcdata->deity = get_deity( ch->pcdata->deity_name ) ) == NULL ) {
                        ch_printf( ch, "Warning: the deity %s no longer exists.\r\n",
                                   ch->pcdata->deity_name );
                        STRFREE( ch->pcdata->deity_name );
                        ch->pcdata->deity_name = STRALLOC( "" );
                        ch->pcdata->favor = 0;
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "Description", ch->description, fread_string( fp ) );

                if ( !strcmp( word, "Dragonform" ) ) {
                    int                     sn;
                    int                     value;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );

                        sn = skill_lookup( fread_word( fp ) );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown dragonform skill/spell." );
                        else
                            ch->pcdata->dlearned[sn] = value;
                        fMatch = TRUE;
                    }
                    break;
                }

                break;
                /*
                 * 'E' was moved to after 'S' 
                 */
            case 'F':
                if ( !strcmp( word, "Firstlevel" ) ) {
                    int                     x = fread_number( fp );

                    if ( x <= 108 )
                        ch->firstlevel = x;
                    else if ( x > 200 )
                        ch->firstlevel = x - 100;
                    else
                        ch->firstlevel = 80;
                    fMatch = TRUE;
                    break;
                }
                KEY( "FirstExpR", ch->firstexpratio, fread_number( fp ) );
                KEY( "Favor", ch->pcdata->favor, fread_number( fp ) );
                if ( !strcmp( word, "Filename" ) ) {
                    fread_to_eol( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Firstexp", ch->firstexp, fread_number( fp ) );
                KEY( "Focuslevel", ch->focus_level, fread_number( fp ) );
                KEY( "Faith", ch->faith, fread_number( fp ) );
                KEY( "Flags", ch->pcdata->flags, fread_number( fp ) );
                if ( !strcmp( word, "FoundArea" ) ) {
                    FOUND_AREA             *area;

                    CREATE( area, FOUND_AREA, 1 );
                    area->area_name = fread_string( fp );
                    if ( area->area_name )
                        LINK( area, ch->pcdata->first_area, ch->pcdata->last_area, next, prev );
                    else
                        DISPOSE( area );
                    fMatch = TRUE;
                    break;
                }
                KEY( "FPrompt", ch->pcdata->fprompt, fread_string( fp ) );
                KEY( "Frostbite", ch->pcdata->frostbite, fread_number( fp ) );
                break;
            case 'G':
                if ( !strcmp( word, "GetsDExp" ) ) {
                    int                     num = fread_number( fp );

                    if ( num == 1 )                    /* DEXP WAS ON WHEN THEY QUIT */
                        ch->pcdata->getsdoubleexp = TRUE;
                    else
                        ch->pcdata->getsdoubleexp = FALSE;
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Glory" ) ) {
                    fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'H':
                KEY( "Heaven_time", ch->pcdata->heaven_time, fread_number( fp ) );
                KEY( "Height", ch->height, fread_number( fp ) );
                if ( !strcmp( word, "Helled" ) ) {
                    ch->pcdata->release_date = fread_number( fp );
                    ch->pcdata->helled_by = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Hitroll", ch->hitroll, fread_number( fp ) );
                KEY( "Holdbreath", ch->pcdata->holdbreath, fread_number( fp ) );
                KEY( "Homepage", ch->pcdata->homepage, fread_string( fp ) );
                if ( !strcmp( word, "HpBloodMove" ) ) {
                    ch->hit = fread_number( fp );
                    ch->max_hit = fread_number( fp );
                    ch->blood = fread_number( fp );
                    ch->max_blood = fread_number( fp );
                    ch->move = fread_number( fp );
                    ch->max_move = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                else if ( !strcmp( word, "HpManaMove" ) ) {
                    ch->hit = fread_number( fp );
                    ch->max_hit = fread_number( fp );
                    ch->mana = fread_number( fp );
                    ch->max_mana = fread_number( fp );
                    ch->move = fread_number( fp );
                    ch->max_move = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Htown" ) ) {
                    ch->pcdata->htown_name = fread_string( fp );
                    if ( !preload && VLD_STR( ch->pcdata->htown_name )
                         && ( ch->pcdata->htown = get_htown( ch->pcdata->htown_name ) ) == NULL ) {
                        ch_printf( ch,
                                   "Warning: the htown %s no longer exists, and herefore you no longer\r\nbelong to a htown.\r\n",
                                   ch->pcdata->htown_name );
                        if ( VLD_STR( ch->pcdata->htown_name ) )
                            STRFREE( ch->pcdata->htown_name );
                    }
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'I':
                KEY( "ICQ", ch->pcdata->icq, fread_number( fp ) );
                if ( !strcmp( word, "Ignored" ) ) {
                    char                   *temp;
                    char                    fname[1024];
                    struct stat             fst;
                    int                     ign;
                    IGNORE_DATA            *inode;

                    /*
                     * Get the name 
                     */
                    temp = fread_string( fp );

                    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( temp[0] ),
                              capitalize( temp ) );

                    /*
                     * If there isn't a pfile for the name 
                     */
                    /*
                     * then don't add it to the list       
                     */
                    if ( stat( fname, &fst ) == -1 ) {
                        if ( temp )
                            STRFREE( temp );
                        fMatch = TRUE;
                        break;
                    }

                    /*
                     * Count the number of names already ignored 
                     */
                    for ( ign = 0, inode = ch->pcdata->first_ignored; inode; inode = inode->next ) {
                        ign++;
                    }

                    /*
                     * Add the name unless the limit has been reached 
                     */
                    if ( ign >= MAX_IGN )
                        bug( "%s", "fread_char: too many ignored names" );
                    else {
                        /*
                         * Add the name to the list 
                         */
                        CREATE( inode, IGNORE_DATA, 1 );
                        inode->name = STRALLOC( temp );
                        inode->next = NULL;
                        inode->prev = NULL;

                        LINK( inode, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next,
                              prev );
                    }
                    if ( temp )
                        STRFREE( temp );

                    fMatch = TRUE;
                    break;
                }
                KEY( "IllegalPK", ch->pcdata->illegal_pk, fread_number( fp ) );
                KEY( "Immune", ch->immune, fread_number( fp ) );
#ifdef I3
                if ( ( fMatch = i3_load_char( ch, fp, word ) ) )
                    break;
#endif
                break;

            case 'K':
                if ( !strcmp( word, "Killed" ) ) {
                    fMatch = TRUE;
                    if ( killcnt >= MAX_KILLTRACK )
                        bug( "fread_char: killcnt (%d) >= MAX_KILLTRACK", killcnt );
                    else {
                        ch->pcdata->killed[killcnt++].vnum = fread_number( fp );
                    }
                }
                break;

            case 'L':
                if ( !str_cmp( word, "LDExpUpdate" ) ) {
                    int                     dexp = fread_number( fp );

                    if ( ch->pcdata->getsdoubleexp == TRUE ) {  /* DExp on when player
                                                                 * quit.. */
                        if ( ( current_time - 3600 ) > dexp ) { /* .. but has since run
                                                                 * out */
                            ch->pcdata->getsdoubleexp = FALSE;
                            ch->pcdata->last_dexpupdate = current_time; /* Half hour til
                                                                         * next solve */
                        }
                        else
                            ch->pcdata->last_dexpupdate = dexp; /* Dexp is still on!
                                                                 * gogogo */
                    }
                    else                               /* DExp was off before, can still
                                                        * * * * start solving in half
                                                        * hour */
                        ch->pcdata->last_dexpupdate = current_time;
                    fMatch = TRUE;
                    break;
                }
                KEY( "LAIR", ch->pcdata->lair, fread_number( fp ) );
                KEY( "LastNews", ch->pcdata->last_read_news, fread_number( fp ) );
                KEY( "Lastrated", ch->pcdata->lastrated, fread_number( fp ) );

                if ( !strcmp( word, "Level" ) ) {
                    int                     x = fread_number( fp );

                    if ( x <= 108 )
                        ch->level = x;
                    else if ( x > 200 )
                        ch->level = x - 100;
                    else
                        ch->level = 80;
                    fMatch = TRUE;
                }

                KEY( "LongDescr", ch->long_descr, fread_string( fp ) );
                if ( !strcmp( word, "Languages" ) ) {
                    ch->speaks = fread_number( fp );
                    ch->speaking = fread_number( fp );
                    fMatch = TRUE;
                }
                break;
            case 'M':
                KEY( "MapToggle", ch->map_toggle, fread_number( fp ) );
                KEY( "MapSize", ch->map_size, fread_number( fp ) );
                KEY( "MapDescToggle", ch->map_desc_toggle, fread_number( fp ) );
                KEY( "MapNameToggle", ch->map_name_toggle, fread_number( fp ) );
                KEY( "MapType", ch->map_type, fread_number( fp ) );
                KEY( "MaxColors", max_colors, fread_number( fp ) );
                KEY( "MDeaths", ch->pcdata->mdeaths, fread_number( fp ) );
                KEY( "Mentalstate", ch->mental_state, fread_number( fp ) );
                if ( !strcmp( word, "MGlory" ) ) {
                    int                     nul = fread_number( fp );

                    nul = 0;
                    fMatch = TRUE;
                    break;
                }
                KEY( "Minsnoop", ch->pcdata->min_snoop, fread_number( fp ) );
                KEY( "MKills", ch->pcdata->mkills, fread_number( fp ) );
                KEY( "Mobinvis", ch->mobinvis, fread_number( fp ) );
                if ( !strcmp( word, "MobRange" ) ) {
                    ch->pcdata->m_range_lo = fread_number( fp );
                    ch->pcdata->m_range_hi = fread_number( fp );
                    fMatch = TRUE;
                }
                if ( !str_cmp( word, "Money" ) ) {
                    x1 = x2 = 0;
                    while ( ( x1 = fread_number( fp ) ) >= 0 && x2 < MAX_CURR_TYPE )
                        GET_MONEY( ch, x2++ ) = x1;
                    GET_MONEY( ch, CURR_NONE ) = 0;
                    fread_to_eol( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "MobCount", ch->arena_mob_count, fread_number( fp ) );
                break;
            case 'N':
                if ( !strcmp( word, "NSong" ) ) {
                    int                     sn,
                                            value;
                    bool                    dshow;

                    value = fread_number( fp );
                    sn = bsearch_skill_exact( fread_word( fp ), gsn_first_song, gsn_top_sn - 1 );
                    dshow = fread_number( fp );

                    if ( sn < 0 )
                        bug( "%s", "Fread_char: unknown song." );
                    else {
                        ch->pcdata->learned[sn] = value;
                        ch->pcdata->dshowlearned[sn] = dshow;
                    }
                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "NNSong" ) ) {
                    int                     sn,
                                            value,
                                            dvalue;
                    bool                    dshow,
                                            ddshow;

                    value = fread_number( fp );
                    dvalue = fread_number( fp );
                    sn = bsearch_skill_exact( fread_word( fp ), gsn_first_song, gsn_top_sn - 1 );
                    dshow = fread_number( fp );
                    ddshow = fread_number( fp );

                    if ( sn < 0 )
                        bug( "%s", "Fread_char: unknown song." );
                    else {
                        ch->pcdata->learned[sn] = value;
                        ch->pcdata->dlearned[sn] = dvalue;
                        ch->pcdata->dshowlearned[sn] = dshow;
                        ch->pcdata->dshowdlearned[sn] = ddshow;
                    }
                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "NSkill" ) ) {
                    char                   *skilltest;
                    int                     sn,
                                            value;
                    bool                    dshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        skilltest = fread_word( fp );
                        dshow = fread_number( fp );

                        sn = bsearch_skill_exact( skilltest, gsn_first_skill, gsn_first_spell - 1 );

                        if ( sn < 0 ) {
                            sn = skill_lookup( skilltest );
                            if ( sn < 0 )
                                bug( "%s: unknown skill (%s).", __FUNCTION__, skilltest );
                            else {
                                ch->pcdata->learned[sn] = value;
                                ch->pcdata->dshowlearned[sn] = dshow;
                            }
                        }
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dshowlearned[sn] = dshow;
                        }
                        fMatch = TRUE;
                        break;
                    }
                }

                if ( !strcmp( word, "NNSkill" ) ) {
                    char                   *skilltest;
                    int                     sn,
                                            value,
                                            dvalue;
                    bool                    dshow,
                                            ddshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        dvalue = fread_number( fp );
                        skilltest = fread_word( fp );
                        dshow = fread_number( fp );
                        ddshow = fread_number( fp );

                        sn = bsearch_skill_exact( skilltest, gsn_first_skill, gsn_first_spell - 1 );

                        if ( sn < 0 ) {
                            sn = skill_lookup( skilltest );
                            if ( sn < 0 )
                                bug( "%s: unknown skill (%s).", __FUNCTION__, skilltest );
                            else {
                                ch->pcdata->learned[sn] = value;
                                ch->pcdata->dlearned[sn] = dvalue;
                                ch->pcdata->dshowlearned[sn] = dshow;
                                ch->pcdata->dshowdlearned[sn] = ddshow;
                            }
                        }
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dlearned[sn] = dvalue;
                            ch->pcdata->dshowlearned[sn] = dshow;
                            ch->pcdata->dshowdlearned[sn] = ddshow;
                        }
                        fMatch = TRUE;
                        break;
                    }
                }

                if ( !strcmp( word, "NSpell" ) ) {
                    char                   *skilltest;
                    int                     sn,
                                            value;
                    bool                    dshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        skilltest = fread_word( fp );
                        dshow = fread_number( fp );

                        sn = skill_lookup( skilltest );
                        if ( sn < 0 )
                            bug( "%s: unknown spell (%s).", __FUNCTION__, skilltest );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dshowlearned[sn] = dshow;
                        }
                        fMatch = TRUE;
                        break;
                    }
                }

                if ( !strcmp( word, "NNSpell" ) ) {
                    char                   *skilltest;
                    int                     sn,
                                            value,
                                            dvalue;
                    bool                    dshow,
                                            ddshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        dvalue = fread_number( fp );
                        skilltest = fread_word( fp );
                        dshow = fread_number( fp );
                        ddshow = fread_number( fp );

                        sn = skill_lookup( skilltest );
                        if ( sn < 0 )
                            bug( "%s: unknown spell (%s).", __FUNCTION__, skilltest );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dlearned[sn] = dvalue;
                            ch->pcdata->dshowlearned[sn] = dshow;
                            ch->pcdata->dshowdlearned[sn] = ddshow;
                        }
                        fMatch = TRUE;
                        break;
                    }
                }

                if ( !strcmp( word, "NTongue" ) ) {
                    int                     sn,
                                            value;
                    bool                    dshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        sn = skill_lookup( fread_word( fp ) );
                        dshow = fread_number( fp );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown tongue." );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dshowlearned[sn] = dshow;

                            if ( ch->level < LEVEL_IMMORTAL )
                                if ( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL ) {
                                    ch->pcdata->learned[sn] = 0;
                                    ch->pcdata->dshowlearned[sn] = FALSE;
                                    ch->practice++;
                                }
                        }
                        fMatch = TRUE;
                    }
                    break;
                }

                if ( !strcmp( word, "NNTongue" ) ) {
                    int                     sn,
                                            value,
                                            dvalue;
                    bool                    dshow,
                                            ddshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        dvalue = fread_number( fp );
                        sn = skill_lookup( fread_word( fp ) );
                        dshow = fread_number( fp );
                        ddshow = fread_number( fp );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown tongue." );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dlearned[sn] = dvalue;
                            ch->pcdata->dshowlearned[sn] = dshow;
                            ch->pcdata->dshowdlearned[sn] = ddshow;

                            /*
                             * Here we will just take away the normal not the dragon ones since they
                             * are temp and could be from life command 
                             */
                            if ( ch->level < LEVEL_IMMORTAL )
                                if ( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL ) {
                                    ch->pcdata->learned[sn] = 0;
                                    ch->pcdata->dshowlearned[sn] = FALSE;
                                    ch->practice++;
                                }
                        }
                        fMatch = TRUE;
                    }
                    break;
                }

                if ( !strcmp( word, "NWeapon" ) ) {
                    int                     sn,
                                            value;
                    bool                    dshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        sn = skill_lookup( fread_word( fp ) );
                        dshow = fread_number( fp );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown weapon." );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dshowlearned[sn] = dshow;
                        }
                        fMatch = TRUE;
                    }
                    break;
                }

                if ( !strcmp( word, "NNWeapon" ) ) {
                    int                     sn,
                                            value,
                                            dvalue;
                    bool                    dshow,
                                            ddshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        dvalue = fread_number( fp );
                        sn = skill_lookup( fread_word( fp ) );
                        dshow = fread_number( fp );
                        ddshow = fread_number( fp );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown weapon." );
                        else {
                            ch->pcdata->learned[sn] = value;
                            ch->pcdata->dlearned[sn] = dvalue;
                            ch->pcdata->dshowlearned[sn] = dshow;
                            ch->pcdata->dshowdlearned[sn] = ddshow;
                        }
                        fMatch = TRUE;
                    }
                    break;
                }

                if ( !strcmp( word, "NDragonform" ) ) {
                    int                     sn,
                                            value;
                    bool                    dshow;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        sn = skill_lookup( fread_word( fp ) );
                        dshow = fread_number( fp );

                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown dragonform skill/spell." );
                        else {
                            ch->pcdata->dlearned[sn] = value;
                            ch->pcdata->dshowdlearned[sn] = dshow;
                        }
                        fMatch = TRUE;
                    }
                    break;
                }

                KEY( "Nboards", ch->pcdata->boards, fread_number( fp ) );
                KEY( "Name", ch->name, fread_string( fp ) );
                KEY( "NoAffectedBy", ch->no_affected_by, fread_bitvector( fp ) );
                /*
                 * KEY("NoAffectedBy2", ch->no_affected_by2, fread_bitvector(fp)); 
                 */
                KEY( "NoImmune", ch->no_immune, fread_number( fp ) );
                KEY( "NoResistant", ch->no_resistant, fread_number( fp ) );
                KEY( "NoSusceptible", ch->no_susceptible, fread_number( fp ) );
                if ( !strcmp( "Nuisance", word ) ) {
                    fMatch = TRUE;
                    CREATE( ch->pcdata->nuisance, NUISANCE_DATA, 1 );
                    ch->pcdata->nuisance->time = fread_number( fp );
                    ch->pcdata->nuisance->max_time = fread_number( fp );
                    ch->pcdata->nuisance->flags = fread_number( fp );
                    ch->pcdata->nuisance->power = 1;
                }
                if ( !strcmp( "NuisanceNew", word ) ) {
                    fMatch = TRUE;
                    CREATE( ch->pcdata->nuisance, NUISANCE_DATA, 1 );
                    ch->pcdata->nuisance->time = fread_number( fp );
                    ch->pcdata->nuisance->max_time = fread_number( fp );
                    ch->pcdata->nuisance->flags = fread_number( fp );
                    ch->pcdata->nuisance->power = fread_number( fp );
                }
                break;
            case 'O':
                if ( !strcmp( word, "ObjRange" ) ) {
                    ch->pcdata->o_range_lo = fread_number( fp );
                    ch->pcdata->o_range_hi = fread_number( fp );
                    fMatch = TRUE;
                }
                KEY( "ObjCount", ch->arena_obj_count, fread_number( fp ) );
                break;
            case 'P':
                KEY( "Pagerlen", ch->pcdata->pagerlen, fread_number( fp ) );
                KEY( "Palace", ch->pcdata->palace, fread_number( fp ) );
                KEY( "Password", ch->pcdata->pwd, fread_string( fp ) );
                KEY( "PDeaths", ch->pcdata->pdeaths, fread_number( fp ) );
                KEY( "PKills", ch->pcdata->pkills, fread_number( fp ) );
                KEY( "Played", ch->played, fread_number( fp ) );
                if ( !strcmp( word, "PKilled" ) ) {
                    PKILLED_DATA           *pkill;

                    pkill = new_pkill(  );
                    pkill->timekilled = fread_number( fp );
                    pkill->name = fread_string( fp );
                    LINK( pkill, ch->pcdata->first_pkill, ch->pcdata->last_pkill, next, prev );
                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "PotionTimer" ) ) {
                    ch->pcdata->potionsoob = fread_number( fp );
                    ch->pcdata->potionspve = fread_number( fp );
                    ch->pcdata->potionspvp = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Position" ) ) {
                    ch->position = fread_number( fp );
                    if ( ch->position < 100 ) {
                        switch ( ch->position ) {
                            default:
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                                break;
                            case 5:
                                ch->position = 6;
                                break;
                            case 6:
                                ch->position = 8;
                                break;
                            case 7:
                                ch->position = 9;
                                break;
                            case 8:
                                ch->position = 12;
                                break;
                            case 9:
                                ch->position = 13;
                                break;
                            case 10:
                                ch->position = 14;
                                break;
                            case 11:
                                ch->position = 15;
                                break;
                        }
                        fMatch = TRUE;
                    }
                    else {
                        ch->position -= 100;
                        fMatch = TRUE;
                    }
                    break;
                }
                KEY( "Practice", ch->practice, fread_number( fp ) );
                KEY( "Preglory_hit", ch->pcdata->preglory_hit, fread_number( fp ) );
                KEY( "Preglory_mana", ch->pcdata->preglory_mana, fread_number( fp ) );
                KEY( "Preglory_move", ch->pcdata->preglory_move, fread_number( fp ) );
                KEY( "Prompt", ch->pcdata->prompt, fread_string( fp ) );
                if ( !strcmp( word, "PTimer" ) ) {
                    add_timer( ch, TIMER_PKILLED, fread_number( fp ), NULL, 0 );
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'Q':
                KEY( "QuestCurr", ch->quest_curr, fread_number( fp ) );
                /*
                 * Volk - should have checked bug/typo/idea code .. 
                 */
                if ( !str_cmp( word, "QuestAccum" ) ) {
                    int                     accum = fread_number( fp );

                    if ( accum < ch->quest_curr )
                        ch->quest_accum = ch->quest_curr;
                    else
                        ch->quest_accum = accum;
                    fMatch = TRUE;
                }
                break;
            case 'R':
                if ( !str_cmp( word, "Race" ) ) {
                    int                     race = fread_number( fp );

                    /*
                     * IF player is a dragon, make them a dragon! 
                     */
                    if ( ch->Class == CLASS_BLACK || ch->Class == CLASS_RED
                         || ch->Class == CLASS_GOLD || ch->Class == CLASS_SILVER
                         || ch->Class == CLASS_BLUE )
                        ch->race = RACE_DRAGON;

                    /*
                     * IF player is a vampire, make them a vampire - same with celestials 
                     */
                    if ( ch->Class == CLASS_ANGEL )
                        ch->race = RACE_CELESTIAL;

                    if ( ch->Class == CLASS_HELLSPAWN )
                        ch->race = RACE_DEMON;

                    else if ( ch->Class == CLASS_VAMPIRE )
                        ch->race = RACE_VAMPIRE;

                    else
                        ch->race = race;

                    fMatch = TRUE;
                    break;
                }
                KEY( "Rank", ch->pcdata->rank, fread_string( fp ) );
                KEY( "Resistant", ch->resistant, fread_number( fp ) );
                KEY( "Restore_time", ch->pcdata->restore_time, fread_number( fp ) );
                if ( !strcmp( word, "Room" ) ) {
                    ch->in_room = get_room_index( fread_number( fp ) );
                    if ( !ch->in_room )
                        ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "RoomRange" ) ) {
                    ch->pcdata->r_range_lo = fread_number( fp );
                    ch->pcdata->r_range_hi = fread_number( fp );
                    fMatch = TRUE;
                }
                KEY( "Rprate", ch->pcdata->rprate, fread_number( fp ) );
                break;
            case 'S':
                KEY( "Secondclass", ch->secondclass, fread_number( fp ) );
                KEY( "Secondexp", ch->secondexp, fread_number( fp ) );
                KEY( "SecondExpR", ch->secondexpratio, fread_number( fp ) );

                if ( !strcmp( word, "Secondlevel" ) ) {
                    int                     x = fread_number( fp );

                    if ( x <= 108 )
                        ch->secondlevel = x;
                    else if ( x > 200 )
                        ch->secondlevel = x - 100;
                    else
                        ch->secondlevel = 80;
                    fMatch = TRUE;
                }

                KEY( "Sex", ch->sex, fread_number( fp ) );
                KEY( "ShortDescr", ch->short_descr, fread_string( fp ) );
                if ( !strcmp( word, "Speed" ) ) {
                    short                   speed;

                    fMatch = TRUE;
                    speed = fread_number( fp );
                    if ( !ch->desc )
                        break;
                    ch->desc->speed = speed;
                    break;
                }
                KEY( "Style", ch->style, fread_number( fp ) );
                KEY( "Susceptible", ch->susceptible, fread_number( fp ) );
                KEY( "SQL", ch->pcdata->sqlnumber, fread_number( fp ) );
                KEY( "SQLpass", ch->pcdata->sqlpass, fread_string( fp ) );
                if ( !strcmp( word, "SavingThrows" ) ) {
                    ch->saving_poison_death = fread_number( fp );
                    ch->saving_wand = fread_number( fp );
                    ch->saving_para_petri = fread_number( fp );
                    ch->saving_breath = fread_number( fp );
                    ch->saving_spell_staff = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Site" ) ) {
                    if ( !preload && !copyover )
                        ch_printf( ch, "Last connected from: %s", fread_word( fp ) );
                    else
                        fread_to_eol( fp );
                    fMatch = TRUE;
                    if ( preload )
                        word = "End";
                    else
                        break;
                }
                if ( !strcmp( word, "Song" ) ) {
                    int                     sn;
                    int                     value;

                    value = fread_number( fp );

                    sn = bsearch_skill_exact( fread_word( fp ), gsn_first_song, gsn_top_sn - 1 );
                    if ( sn < 0 )
                        bug( "%s", "Fread_char: unknown song." );
                    else
                        ch->pcdata->learned[sn] = value;
                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "Skill" ) ) {
                    int                     sn;
                    int                     value;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );
                        char                   *skilltest = fread_word( fp );

                        sn = bsearch_skill_exact( skilltest, gsn_first_skill, gsn_first_spell - 1 );

                        if ( sn < 0 ) {
                            sn = skill_lookup( skilltest );
                            if ( sn < 0 )
                                bug( "%s: unknown skill (%s).", __FUNCTION__, skilltest );
                            else
                                ch->pcdata->learned[sn] = value;
                        }
                        else
                            ch->pcdata->learned[sn] = value;
                        fMatch = TRUE;
                        break;
                    }
                }
                if ( !strcmp( word, "Spell" ) ) {
                    int                     sn;
                    int                     value;
                    char                   *skilltest;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );

//            sn = bsearch_skill_exact(fread_word(fp), gsn_first_spell, gsn_first_skill - 1);
                        skilltest = fread_word( fp );
                        sn = skill_lookup( skilltest );
                        if ( sn < 0 )
                            bug( "%s: unknown spell (%s).", __FUNCTION__, skilltest );
                        else
                            ch->pcdata->learned[sn] = value;
                        fMatch = TRUE;
                        break;
                    }
                }
                if ( strcmp( word, "End" ) )
                    break;
            case 'E':
                if ( !strcmp( word, "End" ) ) {
                    /*
                     * Now the char is fully loaded, check for advanced classes here 
                     */
                    if ( ch->race == RACE_CELESTIAL )
                        ch->Class = CLASS_ANGEL;

                    if ( ch->race == RACE_DEMON && ch->Class != CLASS_BALROG )
                        ch->Class = CLASS_HELLSPAWN;
                    else if ( ch->race == RACE_DEMON && ch->Class == CLASS_BALROG )
                        ch->Class = CLASS_BALROG;

                    if ( xIS_SET( race_table[ch->race]->flags, RACE_ADVANCED ) ) {
                        ch->secondclass = -1;
                        ch->thirdclass = -1;
                    }
                    /*
                     * Some other checks in here for multiclasses.. 
                     */
                    if ( ch->exp < 0 )
                        ch->exp = 0;
                    if ( ch->firstexp < 0 )
                        ch->firstexp = 0;
                    if ( ch->secondexp < 0 )
                        ch->secondexp = 0;
                    if ( ch->thirdexp < 0 )
                        ch->thirdexp = 0;
                    int                     level = 0;

                    if ( !IS_IMMORTAL( ch ) && IS_THIRDCLASS( ch ) ) {

                        if ( ch->level > ( ( ch->firstlevel + ch->secondlevel + ch->thirdlevel ) / 3 ) + 1 ) {  /* Need 
                                                                                                                 * to 
                                                                                                                 * be 
                                                                                                                 * fixed 
                                                                                                                 */
                            level = ch->level + ch->firstlevel + ch->secondlevel + ch->thirdlevel;
                            level /= 4;                /* We'll make this their new level 
                                                        */
                            ch->level = level;
                            ch->firstlevel = level;
                            ch->secondlevel = level;
                            ch->thirdlevel = level;
                            ch->exp = 0;
                            ch->firstexp = 0;
                            ch->secondexp = 0;
                            ch->thirdexp = 0;
                        }
                    }
                    else if ( !IS_IMMORTAL( ch ) && IS_SECONDCLASS( ch ) ) {
                        if ( ch->level > ( ( ch->firstlevel + ch->secondlevel ) / 2 ) + 1 ) {
                            level = ch->level + ch->firstlevel + ch->secondlevel;
                            level /= 3;                /* We'll make this their new level 
                                                        */
                            ch->level = level;
                            ch->firstlevel = level;
                            ch->secondlevel = level;
                            ch->exp = 0;
                            ch->firstexp = 0;
                            ch->secondexp = 0;
                        }
                    }

                    if ( ch->pcdata->chan_listen != NULL ) {
                        MUD_CHANNEL            *channel = NULL;
                        char                   *channels = ch->pcdata->chan_listen;
                        char                    arg[MIL];

                        while ( channels[0] != '\0' ) {
                            channels = one_argument( channels, arg );

                            if ( !( channel = find_channel( arg ) ) )
                                removename( &ch->pcdata->chan_listen, arg );
                        }
                    }
                    /*
                     * Provide at least the one channel - eh, give them ooc too! (Volk)
                     */
                    else {
                        ch->pcdata->chan_listen = STRALLOC( "chat" );
                        ch->pcdata->chan_listen = STRALLOC( "ooc" );
                    }
                    ch->pcdata->editor = NULL;

                    /*
                     * no good for newbies at all 
                     */
                    if ( !IS_IMMORTAL( ch ) && !ch->speaking )
                        ch->speaking = LANG_COMMON;

                    if ( IS_IMMORTAL( ch ) ) {
                        ch->speaks = ~0;
                        if ( ch->speaking == 0 )
                            ch->speaking = ~0;
                    }
                    /*
                     * this disallows chars from being 6', 180lbs, but easier than a flag 
                     */
                    if ( ch->height == 72 )
                        ch->height =
                            number_range( ( int ) ( race_table[ch->race]->height * .9 ),
                                          ( int ) ( race_table[ch->race]->height * 1.1 ) );
                    if ( ch->weight == 180 )
                        ch->weight =
                            number_range( ( int ) ( race_table[ch->race]->weight * .9 ),
                                          ( int ) ( race_table[ch->race]->weight * 1.1 ) );
                    return;
                }
                KEY( "Email", ch->pcdata->email, fread_string( fp ) );
                KEY( "Exp", ch->exp, fread_number( fp ) );
                KEY( "extlevel", ch->pcdata->extlevel, fread_number( fp ) );
                break;
            case 'T':
                KEY( "Textspeed", ch->pcdata->textspeed, fread_number( fp ) );
                KEY( "Thirdclass", ch->thirdclass, fread_number( fp ) );
                KEY( "Thirdexp", ch->thirdexp, fread_number( fp ) );
                KEY( "ThirdExpR", ch->thirdexpratio, fread_number( fp ) );

                if ( !strcmp( word, "Thirdlevel" ) ) {
                    int                     x = fread_number( fp );

                    if ( x <= 108 )
                        ch->thirdlevel = x;
                    else if ( x > 200 )
                        ch->thirdlevel = x - 100;
                    else
                        ch->thirdlevel = 80;
                    fMatch = TRUE;
                }
                KEY( "timezone", ch->pcdata->timezone, fread_number( fp ) );
                KEY( "Tradelevel", ch->pcdata->tradelevel, fread_number( fp ) );
                if ( !strcmp( word, "THYQuest" ) ) {
                    int                     number = fread_number( fp );
                    int                     progress = fread_number( fp );
                    int                     qtimer = fread_number( fp );
                    int                     ctimer = fread_number( fp );
                    int                     times = fread_number( fp );

                    QUEST_DATA             *quest = get_quest_from_number( number );

                    if ( !quest )
                        bug( "Fread_char: No such quest - %d", number );
                    else {
                        CHQUEST_DATA           *chquest;

                        CREATE( chquest, CHQUEST_DATA, 1 );
                        chquest->progress = progress;
                        chquest->questnum = number;
                        chquest->questlimit = qtimer;
                        chquest->times = times;
                        chquest->chaplimit = ctimer;
                        link_chquest( ch, chquest );
                    }
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "THYQuestN" ) ) {
                    int                     number = fread_number( fp );
                    int                     progress = fread_number( fp );
                    int                     qtimer = fread_number( fp );
                    int                     ctimer = fread_number( fp );
                    int                     times = fread_number( fp );
                    int                     kamount = fread_number( fp );

                    QUEST_DATA             *quest = get_quest_from_number( number );

                    if ( !quest )
                        bug( "Fread_char: No such quest - %d", number );
                    else {
                        CHQUEST_DATA           *chquest;

                        CREATE( chquest, CHQUEST_DATA, 1 );
                        chquest->progress = progress;
                        chquest->questnum = number;
                        chquest->questlimit = qtimer;
                        chquest->times = times;
                        chquest->chaplimit = ctimer;
                        chquest->kamount = kamount;
                        link_chquest( ch, chquest );
                    }
                    fMatch = TRUE;
                    break;
                }

                /*
                 * if(!strcmp(word, "Trade")) { int sn; int value;
                 * 
                 * if(preload) word = "End"; else { value = fread_number(fp);
                 * 
                 * // sn = bsearch_skill_exact(fread_word(fp), gsn_first_trade,
                 * gsn_first_song - 1); sn = skill_lookup(fread_word(fp)); if(sn < 0)
                 * bug("%s", "Fread_char: unknown trade."); else ch->pcdata->learned[sn]
                 * = value; fMatch = TRUE; } break; } 
                 */
                KEY( "Tradeclass", ch->pcdata->tradeclass, fread_number( fp ) );
                if ( !strcmp( word, "Tongue" ) ) {
                    int                     sn;
                    int                     value;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );

//            sn = bsearch_skill_exact(fread_word(fp), gsn_first_tongue, gsn_top_sn - 1);
//          sn = bsearch_skill_exact(fread_word(fp), gsn_first_tongue, gsn_first_trade - 1);
                        sn = skill_lookup( fread_word( fp ) );
                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown tongue." );
                        else {
                            ch->pcdata->learned[sn] = value;
                            if ( ch->level < LEVEL_IMMORTAL )
                                if ( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL ) {
                                    ch->pcdata->learned[sn] = 0;
                                    ch->practice++;
                                }
                        }
                        fMatch = TRUE;
                    }
                    break;
                }
/*
        KEY("Trade", ch->pcdata->trade, fread_string(fp));
        KEY("TradeLevel", ch->pcdata->trade_level, fread_number(fp));
*/
                KEY( "Trust", ch->trust, fread_number( fp ) );
                /*
                 * Let no character be trusted higher than one below maxlevel -- Narn 
                 */
                ch->trust = UMIN( ch->trust, MAX_LEVEL - 1 );
                if ( !strcmp( word, "Title" ) ) {
                    ch->pcdata->title = fread_string( fp );
                    if ( isalpha( ch->pcdata->title[0] ) || isdigit( ch->pcdata->title[0] ) ) {
                        snprintf( buf, MSL, " %s", ch->pcdata->title );
                        if ( ch->pcdata->title )
                            STRFREE( ch->pcdata->title );
                        ch->pcdata->title = STRALLOC( buf );
                    }
                    fMatch = TRUE;
                    break;
                }

                KEY( "tmptrust", ch->pcdata->tmptrust, fread_number( fp ) );
                KEY( "tmplevel", ch->pcdata->tmplevel, fread_number( fp ) );
                KEY( "tmpmax_hit", ch->pcdata->tmpmax_hit, fread_number( fp ) );
                KEY( "tmpmax_mana", ch->pcdata->tmpmax_mana, fread_number( fp ) );
                KEY( "tmpmax_move", ch->pcdata->tmpmax_move, fread_number( fp ) );
                KEY( "tmpheight", ch->pcdata->tmpheight, fread_number( fp ) );
                KEY( "tmpcrawl", ch->pcdata->tmpcrawl, fread_number( fp ) );
                KEY( "tmpweight", ch->pcdata->tmpweight, fread_number( fp ) );
                KEY( "tmpclass", ch->pcdata->tmpclass, fread_number( fp ) );
                KEY( "tmprace", ch->pcdata->tmprace, fread_number( fp ) );
                KEY( "tmproom", ch->pcdata->tmproom, fread_number( fp ) );
                break;
            case 'U':
                KEY( "UsedTrade", ch->used_trade, fread_number( fp ) );
                break;
            case 'V':
                KEY( "Vamp_status", ch->pcdata->vamp_status, fread_string( fp ) );
                if ( !strcmp( word, "Vnum" ) ) {
                    ch->pIndexData = get_mob_index( fread_number( fp ) );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Version", file_ver, fread_number( fp ) );
                break;
            case 'W':
                KEY( "Weight", ch->weight, fread_number( fp ) );
                if ( !strcmp( word, "Weapon" ) ) {
                    int                     sn;
                    int                     value;

                    if ( preload )
                        word = "End";
                    else {
                        value = fread_number( fp );

//            sn = bsearch_skill_exact(fread_word(fp), gsn_first_weapon, gsn_first_tongue - 1);
                        sn = skill_lookup( fread_word( fp ) );
                        if ( sn < 0 )
                            bug( "%s", "Fread_char: unknown weapon." );
                        else
                            ch->pcdata->learned[sn] = value;
                        fMatch = TRUE;
                    }
                    break;
                }
                KEY( "Wimpy", ch->wimpy, fread_number( fp ) );
                KEY( "WizInvis", ch->pcdata->wizinvis, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_char: no match: %s", word );
            fread_to_eol( fp );
        }
    }

}

/* Changed so we can tell if there is a problem with an object and need to resave a file */
OBJ_DATA               *fread_obj( CHAR_DATA *ch, FILE * fp, short os_type )
{
    OBJ_DATA               *obj = NULL;
    const char             *word;
    int                     iNest;
    bool                    fMatch;
    bool                    fNest;
    bool                    fVnum;
    ROOM_INDEX_DATA        *room = NULL;

    if ( ch )
        room = ch->in_room;
    CREATE( obj, OBJ_DATA, 1 );

    obj->count = 1;
    obj->wear_loc = -1;
    obj->weight = 1;

    fNest = TRUE;                                      /* Requiring a Nest 0 is a waste */
    fVnum = TRUE;
    iNest = 0;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "ActionDesc", obj->action_desc, fread_string( fp ) );
                if ( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) ) {
                    AFFECT_DATA            *paf;
                    int                     pafmod;

                    CREATE( paf, AFFECT_DATA, 1 );

                    if ( !strcmp( word, "Affect" ) ) {
                        paf->type = fread_number( fp );
                    }
                    else {
                        int                     sn;
                        char                   *skilltest;

                        skilltest = fread_word( fp );
                        sn = skill_lookup( skilltest );
                        if ( sn < -1 )
                            bug( "%s: unknown skill (%s).", __FUNCTION__, skilltest );
                        else if ( sn < 1 )
// Volk: Don't really need a paf->type in here anyway. But could put one.
                            ;
                        else
                            paf->type = sn;
                    }
                    paf->duration = fread_number( fp );
                    pafmod = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_bitvector( fp );
                    if ( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
                         || paf->location == APPLY_STRIPSN || paf->location == APPLY_REMOVESPELL
                         || paf->location == APPLY_RECURRINGSPELL )
                        paf->modifier = slot_lookup( pafmod );
                    else
                        paf->modifier = pafmod;
                    if ( strcmp( a_types[paf->location % REVERSE_APPLY], "unused" ) )
                        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
                    else
                        DISPOSE( paf );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY( "Color", obj->color, fread_number( fp ) );
                KEY( "Cost", obj->cost, fread_number( fp ) );
                KEY( "Count", obj->count, fread_number( fp ) );
                KEY( "Currtype", obj->currtype, fread_number( fp ) );
                break;

            case 'D':
                KEY( "Description", obj->description, fread_string( fp ) );
                break;

            case 'E':
                KEY( "ExtraFlags", obj->extra_flags, fread_bitvector( fp ) );

                if ( !strcmp( word, "ExtraDescr" ) ) {
                    EXTRA_DESCR_DATA       *ed;

                    CREATE( ed, EXTRA_DESCR_DATA, 1 );

                    ed->keyword = fread_string( fp );
                    ed->description = fread_string( fp );
                    LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
                    fMatch = TRUE;
                }

                if ( !strcmp( word, "End" ) ) {
                    if ( !fNest || !fVnum ) {
                        bug( "Fread_obj: %s incomplete object.",
                             VLD_STR( obj->name ) ? obj->name : "NULL" );
                        if ( obj->name )
                            STRFREE( obj->name );
                        if ( obj->description )
                            STRFREE( obj->description );
                        if ( obj->short_descr )
                            STRFREE( obj->short_descr );
                        DISPOSE( obj );
                        return obj;
                    }
                    else {
                        short                   wear_loc = obj->wear_loc;

                        if ( !obj->name )
                            obj->name = QUICKLINK( obj->pIndexData->name );
                        if ( !obj->description )
                            obj->description = QUICKLINK( obj->pIndexData->description );
                        if ( !obj->short_descr )
                            obj->short_descr = QUICKLINK( obj->pIndexData->short_descr );
                        if ( !obj->action_desc )
                            obj->action_desc = QUICKLINK( obj->pIndexData->action_desc );
                        LINK( obj, first_object, last_object, next, prev );
                        obj->pIndexData->count += obj->count;
                        if ( !obj->serial ) {
                            cur_obj_serial = UMAX( ( cur_obj_serial + 1 ) & ( BV30 - 1 ), 1 );
                            obj->serial = obj->pIndexData->serial = cur_obj_serial;
                        }
                        if ( fNest )
                            rgObjNest[iNest] = obj;
                        numobjsloaded += obj->count;
                        ++physicalobjects;
                        if ( file_ver > 1 || obj->wear_loc < -1 || obj->wear_loc >= MAX_WEAR )
                            obj->wear_loc = -1;
                        /*
                         * Corpse saving. -- Altrag 
                         */
                        if ( os_type == OS_CORPSE ) {
                            if ( !room ) {
                                bug( "%s", "Fread_obj: Corpse without room" );
                                room = get_room_index( ROOM_VNUM_LIMBO );
                            }
                            /*
                             * Give the corpse a timer if there isn't one 
                             */

                            if ( obj->timer < 1 )
                                obj->timer = 40;
                            if ( room->vnum == ROOM_VNUM_HALLOFFALLEN && obj->first_content )
                                obj->timer = -1;
                            obj = obj_to_room( obj, room );
                        }
                        else if ( os_type == OS_VAULT ) {
                            if ( ch && ch->in_room )
                                room = ch->in_room;

                            if ( !room ) {
                                bug( "%s: Locker without room", __FUNCTION__ );
                                room = get_room_index( ROOM_VNUM_LIMBO );
                            }
                            if ( room )
                                obj = obj_to_room( obj, room );
                        }
                        else if ( iNest == 0 || rgObjNest[iNest] == NULL ) {
                            int                     slot = -1;
                            bool                    reslot = FALSE;

                            if ( file_ver > 1 && wear_loc > -1 && wear_loc < MAX_WEAR ) {
                                int                     x;

                                for ( x = 0; x < MAX_LAYERS; x++ )
                                    if ( !save_equipment[wear_loc][x] ) {
                                        save_equipment[wear_loc][x] = obj;
                                        slot = x;
                                        reslot = TRUE;
                                        break;
                                    }
                                if ( x == MAX_LAYERS )
                                    bug( "Fread_obj: too many layers %d", wear_loc );
                            }
                            obj = obj_to_char( obj, ch );
                            if ( reslot && slot != -1 )
                                save_equipment[wear_loc][slot] = obj;
                        }
                        else {
                            if ( rgObjNest[iNest - 1] ) {
                                separate_obj( rgObjNest[iNest - 1] );
                                obj = obj_to_obj( obj, rgObjNest[iNest - 1] );
                            }
                            else
                                bug( "Fread_obj: nest layer missing %d", iNest - 1 );
                        }
                        if ( fNest )
                            rgObjNest[iNest] = obj;

//                  combine_affects( obj );
                        return obj;
                    }
                }
                break;

            case 'G':
                KEY( "Glory", obj->glory, fread_number( fp ) );
                break;

            case 'I':
                KEY( "ItemType", obj->item_type, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", obj->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", obj->name, fread_string( fp ) );

                if ( !strcmp( word, "Nest" ) ) {
                    iNest = fread_number( fp );
                    if ( iNest < 0 || iNest >= MAX_NEST ) {
                        bug( "Fread_obj: bad nest %d.", iNest );
                        iNest = 0;
                        fNest = FALSE;
                    }
                    fMatch = TRUE;
                }
                break;

            case 'O':
                KEY( "Owner", obj->owner, fread_string( fp ) );

            case 'R':
                KEY( "Room", room, get_room_index( fread_number( fp ) ) );
                KEY( "Rvnum", obj->room_vnum, fread_number( fp ) );

            case 'S':
                KEY( "ShortDescr", obj->short_descr, fread_string( fp ) );
                KEY( "Size", obj->size, fread_number( fp ) );

                if ( !strcmp( word, "Spell" ) ) {
                    int                     iValue;
                    int                     sn;
                    char                   *skilltest;

                    iValue = fread_number( fp );
                    skilltest = fread_word( fp );
                    sn = skill_lookup( skilltest );
                    if ( iValue < 0 || iValue > 6 )
                        bug( "Fread_obj: bad iValue %d.", iValue );
                    else if ( sn < -1 )
                        bug( "%s: unknown skill (%s).", __FUNCTION__, skilltest );
                    else if ( sn < 1 )                 // Volk: Leaving room for -1
                        // (nothing) and 0 (reserved) - I
                        // know, dirty hack but it's fast.
                        ;
                    else
                        obj->value[iValue] = sn;
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'T':
                KEY( "Timer", obj->timer, fread_number( fp ) );
                break;

            case 'V':
                if ( !strcmp( word, "Values" ) ) {
                    int                     x1,
                                            x2,
                                            x3,
                                            x4,
                                            x5,
                                            x6,
                                            x7;
                    char                   *ln = fread_line( fp );

                    x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
                    sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

                    obj->value[0] = x1;
                    obj->value[1] = x2;
                    obj->value[2] = x3;
                    obj->value[3] = x4;
                    obj->value[4] = x5;
                    obj->value[5] = x6;
                    obj->value[6] = x7;

                    /*
                     * Volk - let's make sure we don't get any bugged sharp blunt weapons hahah! 
                     */
                    if ( ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_PROJECTILE )
                         && !CAN_SHARPEN( obj ) )
                        obj->value[6] == 0;

                    fMatch = TRUE;
                    break;
                }

                if ( !strcmp( word, "Vnum" ) ) {
                    int                     vnum;

                    vnum = fread_number( fp );
                    /*
                     * bug("Fread_obj: bad vnum %d.", vnum);  
                     */
                    if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL ) {
                        bug( "%s: no object for vnum %d.", __FUNCTION__, vnum );
                        fVnum = FALSE;
                    }
                    else {
                        fVnum = TRUE;
                        obj->cost = obj->pIndexData->cost;
                        obj->currtype = obj->pIndexData->currtype;
                        obj->color = obj->pIndexData->color;
                        obj->weight = obj->pIndexData->weight;
                        obj->item_type = obj->pIndexData->item_type;
                        obj->wear_flags = obj->pIndexData->wear_flags;
                        obj->extra_flags = obj->pIndexData->extra_flags;
                    }
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY( "WearFlags", obj->wear_flags, fread_number( fp ) );
                KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
                KEY( "Weight", obj->weight, fread_number( fp ) );
                break;

        }

        if ( !fMatch ) {
            EXTRA_DESCR_DATA       *ed;
            AFFECT_DATA            *paf;

            bug( "Fread_obj: no match. %s", word );
            fread_to_eol( fp );
            if ( obj->name )
                STRFREE( obj->name );
            if ( obj->description )
                STRFREE( obj->description );
            if ( obj->short_descr )
                STRFREE( obj->short_descr );
            while ( ( ed = obj->first_extradesc ) != NULL ) {
                if ( VLD_STR( ed->keyword ) )
                    STRFREE( ed->keyword );
                if ( VLD_STR( ed->description ) )
                    STRFREE( ed->description );
                UNLINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
                DISPOSE( ed );
            }
            while ( ( paf = obj->first_affect ) != NULL ) {
                UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
                DISPOSE( paf );
            }
            DISPOSE( obj );
            return NULL;
        }
    }
}

void set_alarm( long seconds )
{
#ifdef WIN32
    kill_timer(  );                                    /* kill old timer */
    timer_code = timeSetEvent( seconds * 1000L, 1000, alarm_handler, 0, TIME_PERIODIC );
#else
    alarm( seconds );
#endif
}

bool check_parse_name   args( ( char *name, bool newchar ) );

void do_last( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            arg[MIL],
                            name[MIL];
    int                     max = -1,
        cnt = 0,
        days = 0,
        tcount = 0,
        lcount = 0,
        tmsize = 0,
        lmsize = 0,
        tksize = 0,
        lksize = 0,
        tsize = 0,
        lsize = 0;
    DIR                    *dp,
                           *edp;
    struct dirent          *de,
                           *ep;
    struct stat             fst;

    one_argument( argument, arg );
    if ( arg == NULL || arg[0] == '\0' || is_number( arg ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            send_to_char( "Usage: last <player name>\r\n", ch );
            return;
        }
        if ( arg != NULL && is_number( arg ) )
            max = atoi( arg );
        if ( !( dp = opendir( PLAYER_DIR ) ) ) {
            bug( "%s: can't open %s", __FUNCTION__, PLAYER_DIR );
            perror( PLAYER_DIR );
            return;
        }
        while ( ( de = readdir( dp ) ) ) {
            if ( de->d_name[0] == '.' )
                continue;
            snprintf( buf, sizeof( buf ), "%s%s", PLAYER_DIR, de->d_name );
            if ( !( edp = opendir( buf ) ) ) {
                bug( "%s: can't open %s", __FUNCTION__, buf );
                perror( buf );
                closedir( dp );
                dp = NULL;
                return;
            }
            while ( ( ep = readdir( edp ) ) ) {
                if ( ep->d_name[0] == '.' )
                    continue;
                if ( !str_suffix( ".lua", ep->d_name ) ) {
                    lcount++;
                    snprintf( buf, sizeof( buf ), "%s%s/%s", PLAYER_DIR, de->d_name, ep->d_name );
                    if ( stat( buf, &fst ) != -1 ) {
                        lsize += ( int ) fst.st_size;
                        while ( lsize > 1024 ) {
                            lksize++;
                            lsize -= 1024;
                        }
                        while ( lksize > 1024 ) {
                            lmsize++;
                            lksize -= 1024;
                        }
                    }
                    continue;
                }
                snprintf( buf, sizeof( buf ), "%s%s/%s", PLAYER_DIR, de->d_name, ep->d_name );
                if ( stat( buf, &fst ) == -1 )
                    continue;
                tcount++;
                tsize += ( int ) fst.st_size;
                while ( tsize > 1024 ) {
                    tksize++;
                    tsize -= 1024;
                }
                while ( tksize > 1024 ) {
                    tmsize++;
                    tksize -= 1024;
                }
                days = ( ( current_time - fst.st_mtime ) / 86400 );
                snprintf( buf, sizeof( buf ), "%12s(%dK)(%dD)", ep->d_name,
                          ( int ) fst.st_size / 1024, days );
                ch_printf( ch, "%24s", buf );
                if ( ++cnt == 3 || tcount == max ) {
                    cnt = 0;
                    send_to_char( "\r\n", ch );
                }
                if ( tcount == max )
                    break;
            }
            closedir( edp );
            edp = NULL;
            if ( tcount == max )
                break;
        }
        if ( cnt != 0 && tcount != max )
            send_to_char( "\r\n", ch );
        closedir( dp );
        dp = NULL;

        ch_printf( ch, "\r\nPFiles: %d", tcount );
        if ( tmsize > 0 )
            ch_printf( ch, "(%dMB)", tmsize );
        else if ( tksize > 0 )
            ch_printf( ch, "(%dKB)", tksize );
        else
            ch_printf( ch, "(%dB)", tsize );
        ch_printf( ch, " Lua Files: %d", lcount );
        if ( lmsize > 0 )
            ch_printf( ch, "(%dMB)", lmsize );
        else if ( lksize > 0 )
            ch_printf( ch, "(%dKB)", lksize );
        else
            ch_printf( ch, "(%dB)", lsize );
        send_to_char( "\r\n", ch );

        return;
    }
    mudstrlcpy( name, capitalize( arg ), sizeof( name ) );
    snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( arg[0] ), name );
    if ( stat( buf, &fst ) != -1 && check_parse_name( capitalize( name ), FALSE ) )
        ch_printf( ch, "%s(%dK) was last on: %s\r\n", name, ( int ) fst.st_size / 1024,
                   ctime( &fst.st_mtime ) );
    else
        ch_printf( ch, "%s was not found.\r\n", name );
}

/*
 * Added support for removeing so we could take out the write_corpses
 * so we could take it out of the save_char_obj function. --Shaddai
 */

void write_corpses( CHAR_DATA *ch, char *name, OBJ_DATA *objrem )
{
    OBJ_DATA               *corpse;
    FILE                   *fp = NULL;

    /*
     * Name and ch support so that we dont have to have a char to save their
     * corpses.. (ie: decayed corpses while offline) 
     */
    if ( ch && IS_NPC( ch ) ) {
        bug( "%s", "Write_corpses: writing NPC corpse." );
        return;
    }
    if ( ch )
        name = ch->name;
    /*
     * Go by vnum, less chance of screwups. -- Altrag 
     */
    for ( corpse = first_object; corpse; corpse = corpse->next )
        if ( corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && corpse->in_room != NULL
             && !str_cmp( corpse->short_descr + 14, name ) && objrem != corpse ) {
            if ( !fp ) {
                char                    buf[127];

                snprintf( buf, 127, "%s%s", CORPSE_DIR, capitalize( name ) );
                if ( !( fp = FileOpen( buf, "w" ) ) ) {
                    bug( "%s", "Write_corpses: Cannot open file." );
                    perror( buf );
                    return;
                }
            }
            fwrite_obj( ch, corpse, fp, 0, OS_CORPSE, FALSE );
        }
    if ( fp ) {
        fprintf( fp, "#END\n\n" );
        FileClose( fp );
    }
    else {
        char                    buf[127];

        snprintf( buf, 127, "%s%s", CORPSE_DIR, capitalize( name ) );
        remove( buf );
    }
    return;
}

void load_corpses( void )
{
    DIR                    *dp;
    struct dirent          *de;

    if ( !( dp = opendir( CORPSE_DIR ) ) ) {
        bug( "%s", "Load_corpses: can't open CORPSE_DIR" );
        perror( CORPSE_DIR );
        return;
    }

    falling = 1;                                       /* Arbitrary, must be >0 though. */
    while ( ( de = readdir( dp ) ) != NULL ) {
        if ( de->d_name[0] != '.' ) {
            snprintf( strArea, MIL, "%s%s", CORPSE_DIR, de->d_name );
            fprintf( stderr, "Corpse -> %s\n", strArea );
            if ( !( fpArea = FileOpen( strArea, "r" ) ) ) {
                perror( strArea );
                continue;
            }
            for ( ;; ) {
                char                    letter;
                char                   *word;

                letter = fread_letter( fpArea );
                if ( letter == '*' ) {
                    fread_to_eol( fpArea );
                    continue;
                }
                if ( letter != '#' ) {
                    bug( "%s", "Load_corpses: # not found." );
                    break;
                }
                word = fread_word( fpArea );
                if ( !strcmp( word, "CORPSE" ) )
                    fread_obj( NULL, fpArea, OS_CORPSE );
                else if ( !strcmp( word, "OBJECT" ) )
                    fread_obj( NULL, fpArea, OS_CARRY );
                else if ( !strcmp( word, "END" ) )
                    break;
                else {
                    bug( "%s", "Load_corpses: bad section." );
                    break;
                }
            }
            FileClose( fpArea );
        }
    }
    fpArea = NULL;
    mudstrlcpy( strArea, "$", MIL );
    closedir( dp );
    falling = 0;
    return;
}

/* This will write one mobile structure pointed to be fp --Shaddai */
void fwrite_mobile( FILE * fp, CHAR_DATA *mob )
{
    bool                    mounted = FALSE;

    if ( !IS_NPC( mob ) || !fp )
        return;
    fprintf( fp, "#MOBILE\n" );
    fprintf( fp, "Vnum        %d\n", mob->pIndexData->vnum );
    if ( mob->in_room ) {
        fprintf( fp, "Room        %d\n",
                 ( mob->in_room == get_room_index( ROOM_VNUM_LIMBO )
                   && mob->was_in_room ) ? mob->was_in_room->vnum : mob->in_room->vnum );
    }
    if ( VLD_STR( mob->name ) && VLD_STR( mob->pIndexData->player_name )
         && str_cmp( mob->name, mob->pIndexData->player_name ) )
        fprintf( fp, "Name         %s~\n", mob->name );
    if ( VLD_STR( mob->short_descr ) && VLD_STR( mob->pIndexData->short_descr )
         && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
        fprintf( fp, "Short        %s~\n", mob->short_descr );
    if ( VLD_STR( mob->long_descr ) && VLD_STR( mob->pIndexData->long_descr )
         && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
        fprintf( fp, "Long         %s~\n", mob->long_descr );
    if ( VLD_STR( mob->description ) && VLD_STR( mob->pIndexData->description )
         && str_cmp( mob->description, mob->pIndexData->description ) )
        fprintf( fp, "Description  %s~\n", mob->description );

    fprintf( fp, "Level         %d\n", mob->level );
    fprintf( fp, "Hitpoints     %d %d\n", mob->hit, mob->max_hit );
    fprintf( fp, "Mana          %d %d\n", mob->mana, mob->max_mana );
    fprintf( fp, "Move          %d %d\n", mob->move, mob->max_move );
    fprintf( fp, "Hitroll       %d\n", mob->hitroll );
    fprintf( fp, "Damroll       %d\n", mob->damroll );
    fprintf( fp, "Armor         %d\n", mob->armor );

    if ( mob->color )
        fprintf( fp, "Color        %d\n", mob->color );
    if ( area_version > 5 ) {
        if ( VLD_STR( mob->clanname ) && VLD_STR( mob->pIndexData->clanname )
             && str_cmp( mob->clanname, mob->pIndexData->clanname ) )
            fprintf( fp, "Clanname         %s~\n", mob->clanname );
        if ( mob->influence != mob->pIndexData->influence )
            fprintf( fp, "Influence        %d\n", mob->influence );
    }

    fprintf( fp, "Position    %d\n", mob->position );

    if ( xIS_SET( mob->act, ACT_MOUNTED ) ) {
        xREMOVE_BIT( mob->act, ACT_MOUNTED );
        mounted = TRUE;
    }
    fprintf( fp, "Flags       %s\n", print_bitvector( &mob->act ) );
    if ( mounted )
        xSET_BIT( mob->act, ACT_MOUNTED );

/* Might need these later --Shaddai
  de_equip_char(mob);
  re_equip_char(mob);
*/
    if ( mob->first_carrying )
        fwrite_obj( mob, mob->last_carrying, fp, 0, OS_CARRY, FALSE );
    fprintf( fp, "EndMobile\n" );
    return;
}

/*
 * This will read one mobile structure pointer to by fp --Shaddai
 */
CHAR_DATA              *fread_mobile( FILE * fp )
{
    CHAR_DATA              *mob = NULL;
    const char             *word;
    bool                    fMatch;
    int                     inroom = 0;
    ROOM_INDEX_DATA        *pRoomIndex = NULL;

    word = feof( fp ) ? "EndMobile" : fread_word( fp );
    if ( !strcmp( word, "Vnum" ) ) {
        int                     vnum;

        vnum = fread_number( fp );
        mob = create_mobile( get_mob_index( vnum ) );
        if ( !mob ) {
            for ( ;; ) {
                word = feof( fp ) ? "EndMobile" : fread_word( fp );
                /*
                 * So we don't get so many bug messages when something messes up
                 * * --Shaddai 
                 */
                if ( !strcmp( word, "EndMobile" ) )
                    break;
            }
            bug( "Fread_mobile: No index data for vnum %d", vnum );
            return NULL;
        }
    }
    else {
        for ( ;; ) {
            word = feof( fp ) ? "EndMobile" : fread_word( fp );
            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai 
             */
            if ( !strcmp( word, "EndMobile" ) )
                break;
        }
        extract_char( mob, TRUE );
        bug( "%s", "Fread_mobile: Vnum not found" );
        return NULL;
    }
    for ( ;; ) {
        word = feof( fp ) ? "EndMobile" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case '#':
                if ( !strcmp( word, "#OBJECT" ) ) {
                    fread_obj( mob, fp, OS_CARRY );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'A':
                KEY( "Armor", mob->armor, fread_number( fp ) );
                break;

            case 'C':
                if ( !strcmp( word, "Clanname" ) ) {
                    if ( VLD_STR( mob->clanname ) )
                        STRFREE( mob->clanname );
                    mob->clanname = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Color", mob->color, fread_number( fp ) );
                break;

            case 'D':
                if ( !strcmp( word, "Description" ) ) {
                    if ( VLD_STR( mob->description ) )
                        STRFREE( mob->description );
                    mob->description = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Damroll", mob->damroll, fread_number( fp ) );
                break;

            case 'H':
                KEY( "Hitroll", mob->hitroll, fread_number( fp ) );
                if ( !strcmp( word, "Hitpoints" ) ) {
                    mob->hit = fread_number( fp );
                    mob->max_hit = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'E':
                if ( !strcmp( word, "EndMobile" ) ) {
                    if ( inroom == 0 )
                        inroom = ROOM_VNUM_TEMPLE;
                    pRoomIndex = get_room_index( inroom );
                    if ( !pRoomIndex )
                        pRoomIndex = get_room_index( ROOM_VNUM_TEMPLE );
                    char_to_room( mob, pRoomIndex );
                    return mob;
                }
                break;

            case 'F':
                KEY( "Flags", mob->act, fread_bitvector( fp ) );
                break;

            case 'I':
                KEY( "Influence", mob->influence, fread_number( fp ) );
                break;

            case 'L':
                if ( !strcmp( word, "Long" ) ) {
                    if ( VLD_STR( mob->long_descr ) )
                        STRFREE( mob->long_descr );
                    mob->long_descr = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Level", mob->level, fread_number( fp ) );
                break;

            case 'M':
                if ( !strcmp( word, "Mana" ) ) {
                    mob->mana = fread_number( fp );
                    mob->max_mana = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "Move" ) ) {
                    mob->move = fread_number( fp );
                    mob->max_move = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'N':
                if ( !strcmp( word, "Name" ) ) {
                    if ( VLD_STR( mob->name ) )
                        STRFREE( mob->name );
                    mob->name = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'P':
                KEY( "Position", mob->position, fread_number( fp ) );
                break;

            case 'R':
                KEY( "Room", inroom, fread_number( fp ) );
                break;

            case 'S':
                if ( !strcmp( word, "Short" ) ) {
                    if ( VLD_STR( mob->short_descr ) )
                        STRFREE( mob->short_descr );
                    mob->short_descr = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
        }
        if ( !fMatch )
            bug( "Fread_mobile: no match. %s", word );
    }
    return NULL;
}

/*
 * This will write in the saved mobile for a char --Shaddai
 */
void write_char_mobile( CHAR_DATA *ch, char *argument )
{
    FILE                   *fp;
    CHAR_DATA              *mob;

    if ( IS_NPC( ch ) || !ch->pcdata->pet )
        return;

    if ( ( fp = FileOpen( argument, "w" ) ) == NULL ) {
        bug( "Write_char_mobile: couldn't open %s for writing!\r\n", argument );
        return;
    }
    mob = ch->pcdata->pet;
    xSET_BIT( mob->affected_by, AFF_CHARM );
    fwrite_mobile( fp, mob );
    FileClose( fp );
}

/* This will read in the saved mobile for a char --Shaddai */
void read_char_mobile( char *argument )
{
    FILE                   *fp;
    CHAR_DATA              *mob;

    if ( ( fp = FileOpen( argument, "r" ) ) == NULL ) {
        bug( "Read_char_mobile: couldn't open %s for reading!\r\n", argument );
        return;
    }
    mob = fread_mobile( fp );
    FileClose( fp );
}
