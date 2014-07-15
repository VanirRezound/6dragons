 /**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *               Mobile/Player editing module (medit.c)                   *
 *                                                                        *
\**************************************************************************/
#include <ctype.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "h/mud.h"
#include "h/olc.h"
#include "h/files.h"
#include "h/clans.h"
bool                    is_head_architect( CHAR_DATA *ch );
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, const char *txt, int length ) );

/*-------------------------------------------------------------------*/
/* Externals */
extern int              top_mob_index;
extern MOB_INDEX_DATA  *mob_index_hash[MAX_KEY_HASH];

/* Global Variables */
extern const char      *const act_flags[];
char                   *olc_clan_list[50];
char                   *olc_deity_list[50];
char                   *olc_council_list[50];
int                     olc_top_clan;
int                     olc_top_guild;
int                     olc_top_order;
int                     olc_top_council;
int                     olc_top_deity;

const char             *const position_names[] = {
    "dead", "mortallywounded", "incapacitated", "stunned", "sleeping",
    "meditating", "berserk", "resting", "aggressive", "sitting", "crouch", "fighting",
    "defensive", "evasive", "crawl", "standing", "mounted", "shoved", "dragged"
};

/*-------------------------------------------------------------------*/
/*. Function prototypes .*/

int get_actflag         args( ( char *flag ) );
int get_risflag         args( ( char *flag ) );
int get_partflag        args( ( char *flag ) );
int get_attackflag      args( ( char *flag ) );
int get_defenseflag     args( ( char *flag ) );

DECLARE_DO_FUN( do_medit_reset );
void medit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );
void medit_disp_menu    args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_npc_menu args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_pc_menu args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_positions args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_mob_flags args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_plr_flags args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_pcdata_flags args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_aff_flags args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_attack_menu args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_defense_menu args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_ris     args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_spec    args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_clans   args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_deities args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_councils args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_parts   args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_classes args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_races   args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_saving_menu args( ( DESCRIPTOR_DATA *d ) );
void medit_setup_arrays args( ( void ) );

/*-------------------------------------------------------------------*\
  initialization functions
\*-------------------------------------------------------------------*/
void do_omedit( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *victim;
    AREA_DATA              *pArea;
    int                     cvnum,
                            vnum;
    MOB_INDEX_DATA         *pMobIndex;
    char                    buf[MSL];
    CHAR_DATA              *mob;

    if ( IS_NPC( ch ) ) {
        send_to_char( "I don't think so...\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        pArea = ch->in_room->area;
        vnum = pArea->low_m_vnum;

        if ( !pArea ) {
            bug( "mob: !pArea" );
            return;
        }

        while ( vnum <= pArea->hi_m_vnum && get_mob_index( vnum ) != NULL )
            vnum++;
        if ( vnum > pArea->hi_m_vnum ) {
            send_to_char
                ( "&GYou cannot create any more mobs as you have used all that your area is alloted.\r\n",
                  ch );
            return;
        }

        argument = STRALLOC( "mob" );
        pMobIndex = make_mobile( vnum, cvnum, argument );
        pMobIndex->area = ch->pcdata->area;

        if ( !pMobIndex ) {
            send_to_char( "Error.\r\n", ch );
            log_string( "do_mcreate: make_mobile failed." );
            return;
        }
        mob = create_mobile( pMobIndex );

        char_to_room( mob, ch->in_room );
        mudstrlcpy( buf, "mob", MSL );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );
        victim = mob;
    }
    else {
        if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
            send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
            return;
        }
    }

    if ( !IS_NPC( victim ) && get_trust( ch ) < sysdata.level_modify_proto ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    /*
     * Make sure the object isnt already being edited 
     */
    for ( d = first_descriptor; d; d = d->next )
        if ( d->connected == CON_MEDIT )
            if ( d->olc && OLC_VNUM( d ) == victim->pIndexData->vnum ) {
                ch_printf( ch, "That %s is currently being edited by %s.\r\n",
                           IS_NPC( victim ) ? "mobile" : "character", d->character->name );
                return;
            }

    if ( !can_mmodify( ch, victim ) )
        return;

    d = ch->desc;
    CREATE( d->olc, OLC_DATA, 1 );
    if ( IS_NPC( victim ) ) {
        OLC_VNUM( d ) = victim->pIndexData->vnum;
        /*
         * medit_setup( d, OLC_VNUM(d) ); 
         */
    }
    else
        medit_setup_arrays(  );

    d->character->pcdata->dest_buf = victim;
    d->connected = CON_MEDIT;
    OLC_CHANGE( d ) = FALSE;
    medit_disp_menu( d );

    act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_ROOM );
    return;
}

void do_mcopy( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    int                     ovnum,
                            cvnum;
    MOB_INDEX_DATA         *orig;
    MOB_INDEX_DATA         *copy;
    MPROG_DATA             *mprog,
                           *cprog;
    int                     iHash;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !arg1 || !arg2 ) {
        send_to_char( "Usage: mcopy <original> <new>\r\n", ch );
        return;
    }
    ovnum = atoi( arg1 );
    cvnum = atoi( arg2 );
    if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        AREA_DATA              *pArea;

        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to copy objects.\r\n", ch );
            return;
        }
        if ( cvnum < pArea->low_o_vnum || cvnum > pArea->hi_o_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }

    if ( get_mob_index( cvnum ) ) {
        send_to_char( "That object already exists.\r\n", ch );
        return;
    }

    if ( ( orig = get_mob_index( ovnum ) ) == NULL ) {
        send_to_char( "How can you copy something that doesnt exist?\r\n", ch );
        return;
    }

    CREATE( copy, MOB_INDEX_DATA, 1 );
    copy->vnum = cvnum;
    copy->player_name = QUICKLINK( orig->player_name );
    copy->short_descr = QUICKLINK( orig->short_descr );
    copy->long_descr = QUICKLINK( orig->long_descr );
    copy->description = QUICKLINK( orig->description );
    copy->act = orig->act;
    copy->affected_by = orig->affected_by;
    copy->pShop = NULL;
    copy->rShop = NULL;
    copy->spec_fun = orig->spec_fun;
    copy->mudprogs = NULL;
    xCLEAR_BITS( copy->progtypes );
    copy->alignment = orig->alignment;
    copy->level = orig->level;
    copy->mobthac0 = orig->mobthac0;
    copy->ac = orig->ac;
    copy->hitnodice = orig->hitnodice;
    copy->hitsizedice = orig->hitsizedice;
    copy->hitplus = orig->hitplus;
    copy->damnodice = orig->damnodice;
    copy->damsizedice = orig->damsizedice;
    copy->damplus = orig->damplus;
    copy->gold = orig->gold;
    copy->exp = orig->exp;
    copy->position = orig->position;
    copy->defposition = orig->defposition;
    copy->sex = orig->sex;
    copy->perm_str = orig->perm_str;
    copy->perm_dex = orig->perm_dex;
    copy->perm_int = orig->perm_int;
    copy->perm_wis = orig->perm_wis;
    copy->perm_cha = orig->perm_cha;
    copy->perm_con = orig->perm_con;
    copy->perm_lck = orig->perm_lck;
    copy->race = orig->race;
    copy->Class = orig->Class;
    copy->xflags = orig->xflags;
    copy->resistant = orig->resistant;
    copy->immune = orig->immune;
    copy->susceptible = orig->susceptible;
    copy->numattacks = orig->numattacks;
    copy->attacks = orig->attacks;
    copy->defenses = orig->defenses;
    copy->height = orig->height;
    copy->weight = orig->weight;
    copy->saving_poison_death = orig->saving_poison_death;
    copy->saving_wand = orig->saving_wand;
    copy->saving_para_petri = orig->saving_para_petri;
    copy->saving_breath = orig->saving_breath;
    copy->saving_spell_staff = orig->saving_spell_staff;

    if ( orig->mudprogs ) {
        CREATE( mprog, MPROG_DATA, 1 );
        copy->mudprogs = mprog;

        for ( cprog = orig->mudprogs; cprog; cprog = cprog->next ) {
            mprog->type = cprog->type;
            xSET_BIT( copy->progtypes, mprog->type );
            mprog->arglist = QUICKLINK( cprog->arglist );
            mprog->comlist = QUICKLINK( cprog->comlist );
            if ( cprog->next ) {
                CREATE( mprog->next, MPROG_DATA, 1 );
                mprog = mprog->next;
            }
            else
                mprog->next = NULL;
        }
    }

    copy->count = 0;
    iHash = cvnum % MAX_KEY_HASH;
    copy->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = copy;
    top_mob_index++;

    set_char_color( AT_PLAIN, ch );
    send_to_char( "Mobile copied.\r\n", ch );

    return;
}

/*
 * Quite the hack here :P
 */
void medit_setup_arrays( void )
{
    CLAN_DATA              *clan;
    DEITY_DATA             *deity;
    COUNCIL_DATA           *council;
    int                     count;

    count = 0;
    for ( clan = first_clan; clan; clan = clan->next ) {
        olc_clan_list[count] = clan->name;
        count++;
    }
    olc_top_clan = count - 1;

    count = 0;
    for ( deity = first_deity; deity; deity = deity->next ) {
        olc_deity_list[count] = QUICKLINK( deity->name );
        count++;
    }
    olc_top_deity = count;

    count = 0;
    for ( council = first_council; council; council = council->next ) {
        olc_council_list[count] = council->name;
        count++;
    }
    olc_top_council = count;
}

/**************************************************************************
 Menu Displaying Functions
 **************************************************************************/

/*
 * Display poistions (sitting, standing etc), same for pos and defpos
 */
void medit_disp_positions( DESCRIPTOR_DATA *d )
{
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    send_to_char_color( " &c0&w) Dead\r\n", d->character );
    send_to_char_color( " &c1&w) Mortally Wounded\r\n", d->character );
    send_to_char_color( " &c2&w) Incapacitated\r\n", d->character );
    send_to_char_color( " &c3&w) Stunned\r\n", d->character );
    send_to_char_color( " &c4&w) Sleeping\r\n", d->character );
    send_to_char_color( " &c5&w) Meditating\r\n", d->character );
    send_to_char_color( " &c6&w) Berserk\r\n", d->character );
    send_to_char_color( " &c7&w) Resting\r\n", d->character );
    send_to_char_color( " &c8&w) Aggressive\r\n", d->character );
    send_to_char_color( " &c9&w) Sitting\r\n", d->character );
    send_to_char_color( "&c10&w) Crouched\r\n", d->character );
    send_to_char_color( "&c11&w) Fighting\r\n", d->character );
    send_to_char_color( "&c12&w) Defensive\r\n", d->character );
    send_to_char_color( "&c13&w) Evasive\r\n", d->character );
    send_to_char_color( "&c14&w) Crawling\r\n", d->character );
    send_to_char_color( "&c15&w) Standing\r\n", d->character );

    /*
     * for (i = 0; i < POS_MOUNTED; i++) { sprintf(buf, "&g%2d&w) %s\r\n", i,
     * capitalize(position_names[i])); send_to_char_color(buf, d->character); } 
     */
    send_to_char( "Enter position number : ", d->character );
}

/*
 * Display mobile sexes, this is hard coded cause it just works that way :)
 */
void medit_disp_sex( DESCRIPTOR_DATA *d )
{
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    send_to_char_color( " &c0&w) Neutral\r\n", d->character );
    send_to_char_color( " &c1&w) Male\r\n", d->character );
    send_to_char_color( " &c2&w) Female\r\n", d->character );
    send_to_char( "\r\nEnter gender number : ", d->character );
}

void medit_disp_spec( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *ch = d->character;

    ch_printf_color( ch, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( ch, " &c 0&w) No Special\r\n" );
    ch_printf_color( ch, " &c 1&w) Spec_breath_any\r\n" );
    ch_printf_color( ch, " &c 2&w) Spec_breath_acid\r\n" );
    ch_printf_color( ch, " &c 3&w) Spec_breath_fire\r\n" );
    ch_printf_color( ch, " &c 4&w) Spec_breath_frost\r\n" );
    ch_printf_color( ch, " &c 5&w) Spec_breath_gas\r\n" );
    ch_printf_color( ch, " &c 6&w) Spec_breath_lightning\r\n" );
    ch_printf_color( ch, " &c 7&w) Spec_cast_adept\r\n" );
    ch_printf_color( ch, " &c 8&w) Spec_cast_cleric\r\n" );
    ch_printf_color( ch, " &c 9&w) Spec_cast_mage\r\n" );
    ch_printf_color( ch, " &c10&w) Spec_cast_undead\r\n" );
    ch_printf_color( ch, " &c11&w) Spec_executioner\r\n" );
    ch_printf_color( ch, " &c12&w) Spec_fido\r\n" );
    ch_printf_color( ch, " &c13&w) Spec_guard\r\n" );
    ch_printf_color( ch, " &c14&w) Spec_janitor\r\n" );
    ch_printf_color( ch, " &c15&w) Spec_poison\r\n" );
    ch_printf_color( ch, " &c16&w) Spec_thief\r\n" );

    send_to_char( "Enter number of special: ", ch );
}

/*
 * Used for both mob affected_by and object affect bitvectors
 */
void medit_disp_ris( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     counter;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < 21; counter++ ) {
        ch_printf_color( d->character, "&g%2d&w) %-20.20s\r\n", counter + 1, ris_flags[counter] );
    }
    switch ( OLC_MODE( d ) ) {
        case OEDIT_AFFECT_MODIFIER:
            ch_printf_color( d->character, "\r\nCurrent flags: &c%s&w\r\n",
                             flag_string( d->character->tempnum, ris_flags ) );
            break;
        case MEDIT_RESISTANT:
            ch_printf_color( d->character, "\r\nCurrent flags: &c%s&w\r\n",
                             flag_string( victim->resistant, ris_flags ) );
            break;
        case MEDIT_IMMUNE:
            ch_printf_color( d->character, "\r\nCurrent flags: &c%s&w\r\n",
                             flag_string( victim->immune, ris_flags ) );
            break;
        case MEDIT_SUSCEPTIBLE:
            ch_printf_color( d->character, "\r\nCurrent flags: &c%s&w\r\n",
                             flag_string( victim->susceptible, ris_flags ) );
            break;
    }
    send_to_char( "Enter flag (0 to quit): ", d->character );
}

/*
 * Mobile attacks
 */
void medit_disp_attack_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < MAX_ATTACK_TYPE; i++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s\r\n", i + 1, attack_flags[i] );
        send_to_char_color( buf, d->character );
    }
    sprintf( buf, "Current flags: &c%s&w\r\nEnter attack flag (0 to exit): ",
             ext_flag_string( &victim->attacks, attack_flags ) );
    send_to_char_color( buf, d->character );
}

/*
 * Display menu of NPC defense flags
 */
void medit_disp_defense_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < MAX_DEFENSE_TYPE; i++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s\r\n", i + 1, defense_flags[i] );
        send_to_char_color( buf, d->character );
    }
    sprintf( buf, "Current flags: &c%s&w\r\nEnter defense flag (0 to exit): ",
             ext_flag_string( &victim->defenses, defense_flags ) );
    send_to_char_color( buf, d->character );
}

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

void medit_disp_mob_flags( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     i,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < ITEM_PROTOTYPE + 1; i++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s  ", i + 1, act_flags[i] );
        if ( !( ++columns % 2 ) )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    sprintf( buf, "\r\n" "Current flags : &c%s&w\r\n" "Enter mob flags (0 to quit) : ",
             ext_flag_string( &victim->act, act_flags ) );
    send_to_char_color( buf, d->character );
}

/*
 * Special handing for PC only flags
 */
void medit_disp_plr_flags( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     i,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < PLR_INVISPROMPT; i++ ) {
        ch_printf_color( d->character, "&c%2d&w) %-20.20s   ", i + 1, plr_flags[i] );
        if ( ++columns % 2 == 0 )
            send_to_char( "\r\n", d->character );
    }
    sprintf( buf, "\r\nCurrent flags: &c%s&w\r\nEnter flags (0 to quit): ",
             ext_flag_string( &victim->act, plr_flags ) );
    send_to_char_color( buf, d->character );
}

void medit_disp_pcdata_flags( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < 15; i++ ) {
        ch_printf_color( d->character, "&c%2d&w) %-20.20s   \r\n", i + 1, pc_flags[i] );
    }
    ch_printf_color( d->character, "\r\nCurrent flags: &c%s&w\r\nEnter flags (0 to quit): ",
                     flag_string( victim->pcdata->flags, pc_flags ) );
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     i,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < MAX_AFFECTED_BY; i++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s  ", i + 1, a_flags[i] );
        if ( !( ++columns % 2 ) )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    /*
     * sprintf(buf, "\r\n" "Current flags : &c%s&w\r\n" "Enter affected flags (0 to quit) 
     * : ", affect_bit_name( &victim->affected_by ) ); 
     */
    if ( OLC_MODE( d ) == OEDIT_AFFECT_MODIFIER ) {
        buf[0] = '\0';

        for ( i = 0; i < 32; i++ )
            if ( IS_SET( d->character->tempnum, 1 << i ) ) {
                strcat( buf, " " );
                strcat( buf, a_flags[i] );
            }
        ch_printf_color( d->character, "\r\nCurrent flags   : &c%s&w\r\n", buf );
    }
    else
        ch_printf_color( d->character, "\r\nCurrent flags   : &c%s&w\r\n",
                         affect_bit_name( &victim->affected_by ) );
    send_to_char_color( "Enter affected flags (0 to quit) : ", d->character );
}

void medit_disp_clans( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *ch = d->character;
    int                     count;

    ch_printf_color( ch, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    send_to_char( "Clans:\r\n", d->character );
    for ( count = 0; count <= olc_top_clan; count++ ) {
        ch_printf_color( ch, "&c%2d&w) %-20.20s\r\n", count + 1, olc_clan_list[count] );
    }

    send_to_char( "\r\nGuilds:\r\n", d->character );
    for ( count = olc_top_clan + 1; count <= olc_top_guild; count++ ) {
        ch_printf_color( ch, "&c%2d&w) %-20.20s\r\n", count + 1, olc_clan_list[count] );
    }

    send_to_char( "\r\nOrders:\r\n", d->character );
    for ( count = olc_top_guild + 1; count <= olc_top_order; count++ ) {
        ch_printf_color( ch, "&c%2d&w) %-20.20s\r\n", count + 1, olc_clan_list[count] );
    }
    send_to_char( "Enter choice (0 for none): ", d->character );
}

void medit_disp_deities( DESCRIPTOR_DATA *d )
{
    int                     count;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf( d->character, "%d\r\n", olc_top_deity );
    ch_printf_color( d->character, "&c%2d&w) %-20.20s\r\n", 0, "None" );
    for ( count = 0; count < olc_top_deity; count++ ) {
        ch_printf_color( d->character, "&c%2d&w) %-20.20s\r\n", count + 1, olc_deity_list[count] );
    }
    send_to_char( "Enter choice: ", d->character );
}

void medit_disp_councils( DESCRIPTOR_DATA *d )
{
    int                     count;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf( d->character, "%d\r\n", olc_top_council );
    ch_printf_color( d->character, "&c%2d&w) %-20.20s\r\n", 0, "None" );
    for ( count = 0; count < olc_top_council; count++ ) {
        ch_printf_color( d->character, "&c%2d&w) %-20.20s\r\n", count + 1,
                         olc_council_list[count] );
    }
    send_to_char( "Enter choice: ", d->character );
}

void medit_disp_parts( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     count,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );

    for ( count = 0; count < MAX_PARTS; count++ ) {
        ch_printf( d->character, "&g%2d&w) %-20.20s    ", count + 1, part_flags[count] );

        if ( ++columns % 2 == 0 )
            send_to_char( "\r\n", d->character );
    }
    ch_printf( d->character, "\r\nCurrent flags: %s\r\nEnter flag or Q to exit: ",
               ext_flag_string( &victim->xflags, part_flags ) );
}

void medit_disp_classes( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     iClass,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    if ( IS_NPC( victim ) ) {
        for ( iClass = 0; iClass < MAX_NPC_CLASS; iClass++ ) {
            ch_printf_color( d->character, "&c%2d&w) %-20.20s     ", iClass, npc_class[iClass] );
            if ( ++columns % 2 == 0 )
                send_to_char( "\r\n", d->character );
        }
    }
    else {
        for ( iClass = 0; iClass < MAX_CLASS; iClass++ ) {
            ch_printf_color( d->character, "&c%2d&w) %-20.20s     \r\n", iClass,
                             class_table[iClass]->who_name );
            /*
             * if ( ++columns % 2 == 0 ) send_to_char( "\r\n", d->character ); 
             */
        }
    }
    send_to_char( "\r\nEnter class: ", d->character );
}

void medit_disp_races( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     iRace,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    if ( IS_NPC( victim ) ) {
        for ( iRace = 0; iRace < MAX_NPC_RACE; iRace++ ) {
            ch_printf_color( d->character, "&c%2d&w) %-20.20s  ", iRace, npc_race[iRace] );
            if ( ++columns % 3 == 0 )
                send_to_char( "\r\n", d->character );
        }
    }
    else {
        for ( iRace = 0; iRace < MAX_RACE; iRace++ ) {
            ch_printf_color( d->character, "&c%2d&w) %-20.20s     \r\n", iRace,
                             race_table[iRace]->race_name );
            /*
             * if ( ++columns % 2 == 0 ) send_to_char( "\r\n", d->character ); 
             */
        }
    }
    send_to_char( "\r\nEnter race: ", d->character );
}

void medit_disp_saving_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( d->character, "&c1&w) %-30.30s: %2d\r\n", "Saving vs. poison",
                     victim->saving_poison_death );
    ch_printf_color( d->character, "&c2&w) %-30.30s: %2d\r\n", "Saving vs. wands",
                     victim->saving_wand );
    ch_printf_color( d->character, "&c3&w) %-30.30s: %2d\r\n", "Saving vs. paralysis",
                     victim->saving_para_petri );
    ch_printf_color( d->character, "&c4&w) %-30.30s: %2d\r\n", "Saving vs. breath",
                     victim->saving_breath );
    ch_printf_color( d->character, "&c5&w) %-30.30s: %2d\r\n", "Saving vs. spells",
                     victim->saving_spell_staff );
    send_to_char( "\r\nModify which saving throw: ", d->character );

    OLC_MODE( d ) = MEDIT_SAVE_MENU;
}

void medit_disp_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *ch = d->character;
    CHAR_DATA              *mob = ( CHAR_DATA * ) d->character->pcdata->dest_buf;

    if ( !IS_NPC( mob ) ) {
        send_to_char( "Currently 6 Dragons mob Oasis editor is only for mobs, not players.\r\n",
                      ch );
        if ( d->olc ) {
            if ( d->character ) {
                d->character->pcdata->dest_buf = NULL;
            }
            d->connected = CON_PLAYING;
            DISPOSE( d->olc );
        }
        do_silentsave( d->character, ( char * ) "" );
        return;
    }
    // write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
    set_char_color( AT_PLAIN, d->character );
    ch_printf_color( ch, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( ch, "-- Mob Number:  [&c%d&w]\r\n", mob->pIndexData->vnum );
    ch_printf_color( ch, "&c1&w) Sex: &C%-7.7s          &c2&w) Name: &C%s\r\n",
                     mob->sex == SEX_MALE ? "male" : mob->sex == SEX_FEMALE ? "female" : "neutral",
                     mob->name );
    ch_printf_color( ch, "&c3&w) Shortdesc: &C%s\r\n",
                     mob->short_descr[0] == '\0' ? "(none set)" : mob->short_descr );
    ch_printf_color( ch, "&c4&w) Longdesc:-\r\n&C%s\r\n",
                     mob->long_descr[0] == '\0' ? "(none set)" : mob->long_descr );
    ch_printf_color( ch, "&c5&w) Description:-\r\n&C%-74.74s\r\n", mob->description );
    ch_printf_color( ch,
                     "&c6&w) Class: [&C%-11.11s&w], &c7&w) Race:   [&C%-11.11s&w], &c8&w) Level:    [&C%5d&w]\r\n",
                     npc_class[mob->Class], npc_race[mob->race], mob->level );
    ch_printf_color( ch,
                     "&c9&w) Alignment:  [&C%5d&w],  &c@&w) Color:  [ &C%2d&w ],        &cA&w) Strength: [&C%5d&w]\r\n",
                     mob->alignment, mob->color, get_curr_str( mob ) );
    ch_printf_color( ch,
                     "&cB&w) Intelligence:[&C%5d&w], &cC&w) Widsom:       [&C%5d&w], &cD&w) Dexterity:[&C%5d&w]\r\n",
                     get_curr_int( mob ), get_curr_wis( mob ), get_curr_dex( mob ) );
    ch_printf_color( ch,
                     "&cE&w) Constitution:[&C%5d&w], &cF&w) Charisma:     [&C%5d&w], &cG&w) Luck:     [&C%5d&w]\r\n",
                     get_curr_con( mob ), get_curr_cha( mob ), get_curr_lck( mob ) );
    ch_printf( ch,
               "&cH&w) Hitroll: [ &C%d &w]    &cI&w) Damroll: [ &C%d &w]    &cJ&w) Armor: [ &C%d &w]\r\n",
               GET_HITROLL( mob ), GET_DAMROLL( mob ), GET_AC( mob ) );
    ch_printf( ch, "&cK&w) Hitpoints:  [&C%d of %d&w]  &cL&w) NumAttacks: [&C%d&w]\r\n", mob->hit,
               mob->max_hit, mob->numattacks );
    ch_printf_color( ch, "&cM&w) Gold:     [&C%8d&w], &cN&w) Spec: &C%-22.22s\r\n", mob->gold,
                     lookup_spec( mob->spec_fun ) );
    ch_printf_color( ch, "&cO&w) Saving Throws\r\n" );
    ch_printf_color( ch, "&cP&w) Resistant   : &C%s\r\n",
                     flag_string( mob->resistant, ris_flags ) );
    ch_printf_color( ch, "&cR&w) Immune      : &C%s\r\n", flag_string( mob->immune, ris_flags ) );
    ch_printf_color( ch, "&cS&w) Susceptible : &C%s\r\n",
                     flag_string( mob->susceptible, ris_flags ) );
    ch_printf_color( ch, "&cT&w) Position    : &C%s\r\n", position_names[( int ) mob->position] );
    ch_printf_color( ch, "&cU&w) Attacks     : &C%s\r\n",
                     ext_flag_string( &mob->attacks, attack_flags ) );
    ch_printf_color( ch, "&cV&w) Defenses    : &C%s\r\n",
                     ext_flag_string( &mob->defenses, defense_flags ) );
    ch_printf_color( ch, "&cW&w) Body Parts  : &C%s\r\n",
                     ext_flag_string( &mob->xflags, part_flags ) );
    ch_printf_color( ch, "&cX&w) Act Flags   : &C%s\r\n", ext_flag_string( &mob->act, act_flags ) );
    ch_printf_color( ch, "&cY&w) Affected    : &C%s\r\n", affect_bit_name( &mob->affected_by ) );
    ch_printf_color( ch, "&cZ&w) Clear Screen\r\n" );
    ch_printf_color( ch, "&cQ&w) Quit\r\n" );
    ch_printf_color( ch, "Enter choice : " );

    OLC_MODE( d ) = MEDIT_NPC_MAIN_MENU;
}

/*
 * Display main menu for NPCs
 */
void medit_disp_npc_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *ch = d->character;
    CHAR_DATA              *mob = ( CHAR_DATA * ) d->character->pcdata->dest_buf;

    // write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
    set_char_color( AT_PLAIN, d->character );
    ch_printf_color( ch, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( ch, "-- Mob Number:  [&c%d&w]\r\n", mob->pIndexData->vnum );
    ch_printf_color( ch, "&c1&w) Sex: &C%-7.7s          &c2&w) Name: &C%s\r\n",
                     mob->sex == SEX_MALE ? "male" : mob->sex == SEX_FEMALE ? "female" : "neutral",
                     mob->name );
    ch_printf_color( ch, "&c3&w) Shortdesc: &C%s\r\n",
                     mob->short_descr[0] == '\0' ? "(none set)" : mob->short_descr );
    ch_printf_color( ch, "&c4&w) Longdesc:-\r\n&C%s\r\n",
                     mob->long_descr[0] == '\0' ? "(none set)" : mob->long_descr );
    ch_printf_color( ch, "&c5&w) Description:-\r\n&C%-74.74s\r\n", mob->description );
    ch_printf_color( ch,
                     "&c6&w) Class: [&C%-11.11s&w], &c7&w) Race:   [&C%-11.11s&w], &c8&w) Level:    [&C%5d&w]\r\n",
                     npc_class[mob->Class], npc_race[mob->race], mob->level );
    ch_printf_color( ch,
                     "&c9&w) Alignment:  [&C%5d&w],  &c@&w) Color:  [ &C%2d&w ],        &cA&w) Strength: [&C%5d&w]\r\n",
                     mob->alignment, mob->color, get_curr_str( mob ) );
    ch_printf_color( ch,
                     "&cB&w) Intelligence:[&C%5d&w], &cC&w) Widsom:       [&C%5d&w], &cD&w) Dexterity:[&C%5d&w]\r\n",
                     get_curr_int( mob ), get_curr_wis( mob ), get_curr_dex( mob ) );
    ch_printf_color( ch,
                     "&cE&w) Constitution:[&C%5d&w], &cF&w) Charisma:     [&C%5d&w], &cG&w) Luck:     [&C%5d&w]\r\n",
                     get_curr_con( mob ), get_curr_cha( mob ), get_curr_lck( mob ) );
    ch_printf( ch,
               "&cH&w) Hitroll: [ &C%d &w]    &cI&w) Damroll: [ &C%d &w]    &cJ&w) Armor: [ &C%d &w]\r\n",
               GET_HITROLL( mob ), GET_DAMROLL( mob ), GET_AC( mob ) );
    ch_printf( ch, "&cK&w) Hitpoints:  [&C%d of %d&w]  &cL&w) NumAttacks: [&C%d&w]\r\n", mob->hit,
               mob->max_hit, mob->numattacks );
    ch_printf_color( ch, "&cM&w) Gold:     [&C%8d&w], &cN&w) Spec: &C%-22.22s\r\n", mob->gold,
                     lookup_spec( mob->spec_fun ) );
    ch_printf_color( ch, "&cO&w) Saving Throws\r\n" );
    ch_printf_color( ch, "&cP&w) Resistant   : &C%s\r\n",
                     flag_string( mob->resistant, ris_flags ) );
    ch_printf_color( ch, "&cR&w) Immune      : &C%s\r\n", flag_string( mob->immune, ris_flags ) );
    ch_printf_color( ch, "&cS&w) Susceptible : &C%s\r\n",
                     flag_string( mob->susceptible, ris_flags ) );
    ch_printf_color( ch, "&cT&w) Position    : &C%s\r\n", position_names[( int ) mob->position] );
    ch_printf_color( ch, "&cU&w) Attacks     : &C%s\r\n",
                     ext_flag_string( &mob->attacks, attack_flags ) );
    ch_printf_color( ch, "&cV&w) Defenses    : &C%s\r\n",
                     ext_flag_string( &mob->defenses, defense_flags ) );
    ch_printf_color( ch, "&cW&w) Body Parts  : &C%s\r\n",
                     ext_flag_string( &mob->xflags, part_flags ) );
    ch_printf_color( ch, "&cX&w) Act Flags   : &C%s\r\n", ext_flag_string( &mob->act, act_flags ) );
    ch_printf_color( ch, "&cY&w) Affected    : &C%s\r\n", affect_bit_name( &mob->affected_by ) );
    ch_printf_color( ch, "&cZ&w) Clear Screen\r\n" );
    ch_printf_color( ch, "&cQ&w) Quit\r\n" );
    ch_printf_color( ch, "Enter choice : " );

    OLC_MODE( d ) = MEDIT_NPC_MAIN_MENU;
}

void medit_disp_pc_menu( DESCRIPTOR_DATA *d )
{
    CHAR_DATA              *ch = d->character;
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;

    write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
    ch_printf_color( ch, "&g1&w) Sex: &O%-7.7s           &g2&w) Name: &O%s\r\n",
                     victim->sex == SEX_MALE ? "male" : victim->sex ==
                     SEX_FEMALE ? "female" : "neutral", victim->name );
    ch_printf_color( ch, "&g3&w) Description:-\r\n&O%-74.74s\r\n", victim->description );
    ch_printf_color( ch, "&g4&w) Class: [&c%-11.11s&w],  &g5&w) Race:   [&c%-11.11s&w]\r\n",
                     class_table[victim->Class]->who_name, race_table[victim->race]->race_name );
    ch_printf_color( ch,
                     "&g6&w) Level:       [&c%5d&w],  &g7&w) Alignment:    [&c%5d&w],  &g8&w) Strength:  [&c%5d&w]\r\n",
                     victim->level, victim->alignment, get_curr_str( victim ) );
    ch_printf_color( ch,
                     "&g9&w) Intelligence:[&c%5d&w],  &gA&w) Widsom:       [&c%5d&w],  &gB&w) Dexterity: [&c%5d&w]\r\n",
                     get_curr_int( victim ), get_curr_wis( victim ), get_curr_dex( victim ) );
    ch_printf_color( ch,
                     "&gC&w) Constitution:[&c%5d&w],  &gD&w) Charisma:     [&c%5d&w],  &gE&w) Luck:      [&c%5d&w]\r\n",
                     get_curr_con( victim ), get_curr_cha( victim ), get_curr_lck( victim ) );
    ch_printf_color( ch,
                     "&gF&w) Hps:   [&c%5d&w/&c%5d&w],  &gG&w) Mana:   [&c%5d&w/&c%5d&w],  &gH&w) Move:[&c%5d&w/&c%-5d&w]\r\n",
                     victim->hit, victim->max_hit, victim->mana, victim->max_mana, victim->move,
                     victim->max_move );
    ch_printf_color( ch,
                     "&gI&w) Gold:  [&c%11d&w],  &gJ&w) Mentalstate:  [&c%5d&w],  &gK&w) Emotional: [&c%5d&w]\r\n",
                     victim->gold, victim->mental_state, victim->emotional_state );
    ch_printf_color( ch,
                     "&gL&w) Thirst:      [&c%5d&w],  &gM&w) Full:         [&c%5d&w],  &gN&w) Drunk:     [&c%5d&w]\r\n",
                     victim->pcdata->condition[COND_THIRST], victim->pcdata->condition[COND_FULL],
                     victim->pcdata->condition[COND_DRUNK] );
    ch_printf_color( ch, "&gO&w) Favor:       [&c%5d&w]\r\n", victim->pcdata->favor );
    ch_printf_color( ch, "&gP&w) Saving Throws\r\n" );
    ch_printf_color( ch, "&gR&w) Resistant   : &O%s\r\n",
                     flag_string( victim->resistant, ris_flags ) );
    ch_printf_color( ch, "&gS&w) Immune      : &O%s\r\n",
                     flag_string( victim->immune, ris_flags ) );
    ch_printf_color( ch, "&gT&w) Susceptible : &O%s\r\n",
                     flag_string( victim->susceptible, ris_flags ) );
    ch_printf_color( ch, "&gU&w) Position    : &O%s\r\n",
                     position_names[( int ) victim->position] );
    ch_printf_color( ch, "&gV&w) Act Flags   : &c%s\r\n",
                     ext_flag_string( &victim->act, plr_flags ) );
    ch_printf_color( ch, "&gW&w) PC Flags    : &c%s\r\n",
                     flag_string( victim->pcdata->flags, pc_flags ) );
    ch_printf_color( ch, "&gX&w) Affected    : &c%s\r\n", affect_bit_name( &victim->affected_by ) );
    ch_printf_color( ch, "&gY&w) Deity       : &O%s\r\n",
                     victim->pcdata->deity ? victim->pcdata->deity->name : "None" );

    if ( get_trust( ch ) >= LEVEL_IMMORTAL && victim->pcdata->clan )
        ch_printf_color( ch, "&gZ&w) &O%s\r\n", victim->pcdata->clan->name );
    else if ( get_trust( ch ) >= LEVEL_IMMORTAL && !victim->pcdata->clan )
        ch_printf_color( ch, "&gZ&w) Clan        : &ONone\r\n" );

    if ( get_trust( ch ) >= LEVEL_AJ_SGT )
        ch_printf_color( ch, "&g=&w) Council     : &O%s\r\n",
                         victim->pcdata->council ? victim->pcdata->council->name : "None" );
    ch_printf_color( ch, "&gQ&w) Quit\r\n" );
    ch_printf_color( ch, "Enter choice : " );

    OLC_MODE( d ) = MEDIT_PC_MAIN_MENU;
}

/*
 * Bogus command for resetting stuff
 */
void do_medit_reset( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) ch->pcdata->dest_buf;

    switch ( ch->substate ) {
        default:
            return;

        case SUB_MOB_DESC:
            if ( !ch->pcdata->dest_buf ) {
                send_to_char( "Fatal error, report to Tagith.\r\n", ch );
                bug( "do_medit_reset: sub_mob_desc: NULL ch->pcdata->dest_buf" );
                cleanup_olc( ch->desc );
                ch->substate = SUB_NONE;
                return;
            }
            STRFREE( victim->description );
            victim->description = copy_buffer( ch );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->description );
                victim->pIndexData->description = QUICKLINK( victim->description );
            }
            stop_editing( ch );
            ch->pcdata->dest_buf = victim;
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_MEDIT;
            medit_disp_menu( ch->desc );
            return;
    }
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void medit_parse( DESCRIPTOR_DATA *d, char *arg )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    int                     number = 0,
        minattr,
        maxattr;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    CLAN_DATA              *clan;
    DEITY_DATA             *deity;
    COUNCIL_DATA           *council;
    AREA_DATA              *tarea;
    bool                    found = FALSE;
    char                   *pwdnew,
                           *p;

    if ( IS_NPC( victim ) ) {
        minattr = 1;
        maxattr = 25;
    }
    else {
        minattr = 3;
        maxattr = 18;
    }

    switch ( OLC_MODE( d ) ) {
        case MEDIT_CONFIRM_SAVESTRING:
            switch ( UPPER( *arg ) ) {
                case 'Y':
                    /*
                     * If its a mob, fold_area otherwise save_char_obj 
                     */
                    send_to_char( "Saving...\r\n", d->character );
                    if ( IS_NPC( victim ) ) {
                        sprintf( log_buf, "OLC: %s edits mob %d(%s)", d->character->name,
                                 victim->pIndexData->vnum, victim->name );
                        for ( tarea = first_asort; tarea; tarea = tarea->next ) {
                            if ( OLC_VNUM( d ) >= tarea->low_m_vnum
                                 && OLC_VNUM( d ) <= tarea->hi_m_vnum ) {
                                if ( get_trust( d->character ) >= LEVEL_AJ_SGT )
                                    fold_area( tarea, tarea->filename, FALSE );
                                found = TRUE;
                            }
                        }
                        /*
                         * I'm assuming that if it isn't an installed area, its the
                         * char's 
                         */
                        if ( !found && ( tarea = d->character->pcdata->area ) != NULL
                             && get_trust( d->character ) >= LEVEL_IMMORTAL
                             && IS_SET( tarea->status, AREA_LOADED ) ) {
                            tarea = d->character->pcdata->area;
                            sprintf( buf, "%s%s", BUILD_DIR, tarea->filename );
                            fold_area( tarea, buf, FALSE );
                        }
                    }
                    else {
                        sprintf( log_buf, "OLC: %s edits %s", d->character->name, victim->name );
                        save_char_obj( victim );
                    }
                    log_string_plus( log_buf, LOG_BUILD, d->character->level );
                    cleanup_olc( d );
                    return;
                case 'N':
                    cleanup_olc( d );
                    return;
                default:
                    send_to_char( "Invalid choice!\r\n", d->character );
                    send_to_char( "Do you wish to save to disk? : ", d->character );
                    return;
            }
            break;

        case MEDIT_NPC_MAIN_MENU:
            switch ( UPPER( *arg ) ) {
                case 'Q':
                    cleanup_olc( d );
                    return;
                case '1':
                    OLC_MODE( d ) = MEDIT_SEX;
                    medit_disp_sex( d );
                    return;
                case '2':
                    OLC_MODE( d ) = MEDIT_NAME;
                    send_to_char( "\r\nEnter name: ", d->character );
                    return;
                case '3':
                    OLC_MODE( d ) = MEDIT_S_DESC;
                    send_to_char( "\r\nEnter short description: ", d->character );
                    return;
                case '4':
                    OLC_MODE( d ) = MEDIT_L_DESC;
                    send_to_char( "\r\nEnter long description: ", d->character );
                    return;
                case '5':
                    OLC_MODE( d ) = MEDIT_D_DESC;
                    d->character->substate = SUB_MOB_DESC;
                    d->character->last_cmd = do_medit_reset;

                    send_to_char( "Enter new mob description:\r\n", d->character );
                    if ( !victim->description )
                        victim->description = STRALLOC( "" );
                    start_editing( d->character, victim->description );
                    return;
                case '6':
                    OLC_MODE( d ) = MEDIT_CLASS;
                    medit_disp_classes( d );
                    return;
                case '7':
                    OLC_MODE( d ) = MEDIT_RACE;
                    medit_disp_races( d );
                    return;
                case '8':
                    OLC_MODE( d ) = MEDIT_LEVEL;
                    send_to_char( "\r\nEnter level: ", d->character );
                    return;
                case '9':
                    OLC_MODE( d ) = MEDIT_ALIGNMENT;
                    send_to_char( "\r\nEnter alignment: ", d->character );
                    return;
                case '@':
                    {
                        OLC_MODE( d ) = MEDIT_COLOR;
                        int                     counter,
                                                col = 0;

                        send_to_char( "\r\nAvailable mob colors\r\n", d->character );
                        for ( counter = 0; counter < MAX_NUMBER; counter++ ) {
                            ch_printf( d->character, "&c%2d&w) %-20.20s ", counter,
                                       freak_color[counter] );
                            if ( ++col % 2 == 0 )
                                send_to_char( "\r\n", d->character );
                        }
                        send_to_char
                            ( "\r\nEnter the number for the corresponding color of the mob: ",
                              d->character );
                        return;
                    }
                case 'A':
                    OLC_MODE( d ) = MEDIT_STRENGTH;
                    send_to_char( "\r\nEnter strength: ", d->character );
                    return;
                case 'B':
                    OLC_MODE( d ) = MEDIT_INTELLIGENCE;
                    send_to_char( "\r\nEnter intelligence: ", d->character );
                    return;
                case 'C':
                    OLC_MODE( d ) = MEDIT_WISDOM;
                    send_to_char( "\r\nEnter wisdom: ", d->character );
                    return;
                case 'D':
                    OLC_MODE( d ) = MEDIT_DEXTERITY;
                    send_to_char( "\r\nEnter dexterity: ", d->character );
                    return;
                case 'E':
                    OLC_MODE( d ) = MEDIT_CONSTITUTION;
                    send_to_char( "\r\nEnter constitution: ", d->character );
                    return;
                case 'F':
                    OLC_MODE( d ) = MEDIT_CHARISMA;
                    send_to_char( "\r\nEnter charisma: ", d->character );
                    return;
                case 'G':
                    OLC_MODE( d ) = MEDIT_LUCK;
                    send_to_char( "\r\nEnter luck: ", d->character );
                    return;
                case 'H':
                    OLC_MODE( d ) = MEDIT_HITROLL;
                    send_to_char( "\r\nEnter number of hitroll: ", d->character );
                    return;
                case 'I':
                    OLC_MODE( d ) = MEDIT_DAMROLL;
                    send_to_char( "\r\nEnter size of damroll: ", d->character );
                    return;
                case 'J':
                    OLC_MODE( d ) = MEDIT_AC;
                    send_to_char( "\r\nEnter number of armor class: ", d->character );
                    return;
                case 'K':
                    OLC_MODE( d ) = MEDIT_HITPOINT;
                    send_to_char( "\r\nEnter number of hitpoints: ", d->character );
                    return;
                case 'L':
                    OLC_MODE( d ) = MEDIT_NUMATTACKS;
                    send_to_char( "\r\nEnter number of attacks: ", d->character );
                    return;
                case 'M':
                    OLC_MODE( d ) = MEDIT_GOLD;
                    send_to_char( "\r\nEnter amount of gold: ", d->character );
                    return;
                case 'N':
                    OLC_MODE( d ) = MEDIT_SPEC;
                    medit_disp_spec( d );
                    return;
                case 'O':
                    OLC_MODE( d ) = MEDIT_SAVE_MENU;
                    medit_disp_saving_menu( d );
                    return;
                case 'P':
                    OLC_MODE( d ) = MEDIT_RESISTANT;
                    medit_disp_ris( d );
                    return;
                case 'R':
                    OLC_MODE( d ) = MEDIT_IMMUNE;
                    medit_disp_ris( d );
                    return;
                case 'S':
                    OLC_MODE( d ) = MEDIT_SUSCEPTIBLE;
                    medit_disp_ris( d );
                    return;
                case 'T':
                    OLC_MODE( d ) = MEDIT_POS;
                    medit_disp_positions( d );
                    return;
                case 'U':
                    OLC_MODE( d ) = MEDIT_ATTACK;
                    medit_disp_attack_menu( d );
                    return;
                case 'V':
                    OLC_MODE( d ) = MEDIT_DEFENSE;
                    medit_disp_defense_menu( d );
                    return;
                case 'W':
                    OLC_MODE( d ) = MEDIT_PARTS;
                    medit_disp_parts( d );
                    return;
                case 'X':
                    OLC_MODE( d ) = MEDIT_NPC_FLAGS;
                    medit_disp_mob_flags( d );
                    return;
                case 'Y':
                    OLC_MODE( d ) = MEDIT_AFF_FLAGS;
                    medit_disp_aff_flags( d );
                    return;
                case 'Z':
                    write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
                    medit_disp_npc_menu( d );
                    return;
                default:
                    medit_disp_npc_menu( d );
                    return;
            }
            break;

        case MEDIT_PC_MAIN_MENU:
            switch ( UPPER( *arg ) ) {
                case 'Q':
                    if ( OLC_CHANGE( d ) ) {
                        send_to_char( "Do you wish to save changes to disk? (y/n): ",
                                      d->character );
                        OLC_MODE( d ) = MEDIT_CONFIRM_SAVESTRING;
                    }
                    else
                        cleanup_olc( d );
                    return;
                case '1':
                    OLC_MODE( d ) = MEDIT_SEX;
                    medit_disp_sex( d );
                    return;
                case '2':
                    OLC_MODE( d ) = MEDIT_NAME;
                    return;
                case '3':
                    OLC_MODE( d ) = MEDIT_D_DESC;
                    d->character->substate = SUB_MOB_DESC;
                    d->character->last_cmd = do_medit_reset;

                    send_to_char( "Enter new player description:\r\n", d->character );
                    if ( !victim->description )
                        victim->description = STRALLOC( "" );
                    start_editing( d->character, victim->description );
                    return;
                case '4':
                    OLC_MODE( d ) = MEDIT_CLASS;
                    medit_disp_classes( d );
                    return;
                case '5':
                    OLC_MODE( d ) = MEDIT_RACE;
                    medit_disp_races( d );
                    return;
                case '6':
                    send_to_char( "\r\nNPC Only!!", d->character );
                    break;
                case '7':
                    OLC_MODE( d ) = MEDIT_ALIGNMENT;
                    send_to_char( "\r\nEnter alignment: ", d->character );
                    return;
                case '8':
                    OLC_MODE( d ) = MEDIT_STRENGTH;
                    send_to_char( "\r\nEnter strength: ", d->character );
                    return;
                case '9':
                    OLC_MODE( d ) = MEDIT_INTELLIGENCE;
                    send_to_char( "\r\nEnter intelligence: ", d->character );
                    return;
                case 'A':
                    OLC_MODE( d ) = MEDIT_WISDOM;
                    send_to_char( "\r\nEnter wisdom: ", d->character );
                    return;
                case 'B':
                    OLC_MODE( d ) = MEDIT_DEXTERITY;
                    send_to_char( "\r\nEnter dexterity: ", d->character );
                    return;
                case 'C':
                    OLC_MODE( d ) = MEDIT_CONSTITUTION;
                    send_to_char( "\r\nEnter constitution: ", d->character );
                    return;
                case 'D':
                    OLC_MODE( d ) = MEDIT_CHARISMA;
                    send_to_char( "\r\nEnter charisma: ", d->character );
                    return;
                case 'E':
                    OLC_MODE( d ) = MEDIT_LUCK;
                    send_to_char( "\r\nEnter luck: ", d->character );
                    return;
                case 'F':
                    OLC_MODE( d ) = MEDIT_HITPOINT;
                    send_to_char( "\r\nEnter hitpoints: ", d->character );
                    return;
                case 'G':
                    OLC_MODE( d ) = MEDIT_MANA;
                    send_to_char( "\r\nEnter mana: ", d->character );
                    return;
                case 'H':
                    OLC_MODE( d ) = MEDIT_MOVE;
                    send_to_char( "\r\nEnter moves: ", d->character );
                    return;
                case 'I':
                    OLC_MODE( d ) = MEDIT_GOLD;
                    send_to_char( "\r\nEnter amount of gold player carries: ", d->character );
                    return;
                case 'J':
                    OLC_MODE( d ) = MEDIT_MENTALSTATE;
                    send_to_char( "\r\nEnter players mentalstate: ", d->character );
                    return;
                case 'K':
                    OLC_MODE( d ) = MEDIT_EMOTIONAL;
                    send_to_char( "\r\nEnter players emotional state: ", d->character );
                    return;
                case 'L':
                    OLC_MODE( d ) = MEDIT_THIRST;
                    send_to_char( "\r\nEnter player's thirst (0 = dehydrated): ", d->character );
                    return;
                case 'M':
                    OLC_MODE( d ) = MEDIT_FULL;
                    send_to_char( "\r\nEnter player's fullness (0 = starving): ", d->character );
                    return;
                case 'N':
                    OLC_MODE( d ) = MEDIT_DRUNK;
                    send_to_char( "\r\nEnter player's drunkeness (0 = sober): ", d->character );
                    return;
                case 'O':
                    OLC_MODE( d ) = MEDIT_FAVOR;
                    send_to_char( "\r\nEnter player's favor (-2500 to 2500): ", d->character );
                    return;
                case 'P':
                    OLC_MODE( d ) = MEDIT_SAVE_MENU;
                    medit_disp_saving_menu( d );
                    return;
                case 'R':
                    OLC_MODE( d ) = MEDIT_RESISTANT;
                    medit_disp_ris( d );
                    return;
                case 'S':
                    OLC_MODE( d ) = MEDIT_IMMUNE;
                    medit_disp_ris( d );
                    return;
                case 'T':
                    OLC_MODE( d ) = MEDIT_SUSCEPTIBLE;
                    medit_disp_ris( d );
                    return;
                case 'U':
                    send_to_char( "NPCs Only!!\r\n", d->character );
                    break;
                case 'V':
                    OLC_MODE( d ) = MEDIT_PC_FLAGS;
                    medit_disp_plr_flags( d );
                    return;
                case 'W':
                    OLC_MODE( d ) = MEDIT_PCDATA_FLAGS;
                    medit_disp_pcdata_flags( d );
                    return;
                case 'X':
                    OLC_MODE( d ) = MEDIT_AFF_FLAGS;
                    medit_disp_aff_flags( d );
                    return;
                case 'Y':
                    OLC_MODE( d ) = MEDIT_DEITY;
                    medit_disp_deities( d );
                    return;
                case 'Z':
                    if ( get_trust( d->character ) < LEVEL_IMMORTAL )
                        break;
                    OLC_MODE( d ) = MEDIT_CLAN;
                    medit_disp_clans( d );
                    return;
                case '=':
                    if ( get_trust( d->character ) < LEVEL_IMMORTAL )
                        break;
                    OLC_MODE( d ) = MEDIT_COUNCIL;
                    medit_disp_councils( d );
                    return;
                default:
                    medit_disp_npc_menu( d );
                    return;
            }
            break;

        case MEDIT_NAME:
            if ( !IS_NPC( victim ) && get_trust( d->character ) > LEVEL_IMMORTAL ) {
                sprintf( buf, "%s %s", victim->name, arg );
                do_pcrename( d->character, buf );
                olc_log( d, "Changes name to %s", arg );
                return;
            }
            STRFREE( victim->name );
            victim->name = STRALLOC( arg );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->player_name );
                victim->pIndexData->player_name = QUICKLINK( victim->name );
            }
            olc_log( d, "Changed name to %s", arg );
            break;

        case MEDIT_S_DESC:
            STRFREE( victim->short_descr );
            victim->short_descr = STRALLOC( arg );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->short_descr );
                victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
            }
            olc_log( d, "Changed short desc to %s", arg );
            break;

        case MEDIT_L_DESC:
            STRFREE( victim->long_descr );
            strcpy( buf, arg );
            strcat( buf, "\r\n" );
            victim->long_descr = STRALLOC( buf );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
                STRFREE( victim->pIndexData->long_descr );
                victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
            }
            olc_log( d, "Changed long desc to %s", arg );
            break;

        case MEDIT_D_DESC:
            /*
             * . We should never get here .
             */
            cleanup_olc( d );
            bug( "OLC: medit_parse(): Reached D_DESC case!" );
            break;

        case MEDIT_NPC_FLAGS:
            /*
             * REDONE, again, then again 
             */
            if ( is_number( arg ) )
                if ( atoi( arg ) == 0 )
                    break;

            while ( arg[0] != '\0' ) {
                arg = one_argument( arg, arg1 );

                if ( is_number( arg1 ) ) {
                    number = atoi( arg1 );
                    number -= 1;

                    if ( number < 0 || number > 31 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                }
                else {
                    number = get_actflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                }
                if ( IS_NPC( victim ) && number == ACT_PROTOTYPE
                     && get_trust( d->character ) < LEVEL_AJ_SGT
                     && !is_name( "protoflag", d->character->pcdata->bestowments ) )
                    send_to_char( "You don't have permission to change the prototype flag.\r\n",
                                  d->character );
                else if ( IS_NPC( victim ) && number == ACT_IS_NPC )
                    send_to_char( "It isn't possible to change that flag.\r\n", d->character );
                else {
                    xTOGGLE_BIT( victim->act, number );
                }
                if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                    victim->pIndexData->act = victim->act;
            }
            medit_disp_mob_flags( d );
            return;

        case MEDIT_PC_FLAGS:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;
                if ( ( number > 0 ) || ( number < 31 ) ) {
                    number -= 1;                       /* offset :P */
                    xTOGGLE_BIT( victim->act, number );
                    olc_log( d, "%s the flag %s",
                             xIS_SET( victim->act, number ) ? "Added" : "Removed",
                             plr_flags[number] );
                }
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_actflag( arg1 );
                    if ( number > 0 ) {
                        xTOGGLE_BIT( victim->act, number );
                        olc_log( d, "%s the flag %s",
                                 xIS_SET( victim->act, number ) ? "Added" : "Removed",
                                 plr_flags[number] );
                    }
                }
            }
            medit_disp_plr_flags( d );
            return;

        case MEDIT_PCDATA_FLAGS:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;
                if ( ( number > 0 ) || ( number < 31 ) ) {
                    number -= 1;
                    TOGGLE_BIT( victim->pcdata->flags, 1 << number );
                    olc_log( d, "%s the pcflag %s",
                             IS_SET( victim->pcdata->flags, 1 << number ) ? "Added" : "Removed",
                             pc_flags[number] );
                }
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_actflag( arg1 );
                    if ( number > 0 ) {
                        TOGGLE_BIT( victim->pcdata->flags, 1 << number );
                        olc_log( d, "%s the pcflag %s",
                                 IS_SET( victim->pcdata->flags, 1 << number ) ? "Added" : "Removed",
                                 pc_flags[number] );
                    }
                }
            }
            medit_disp_pcdata_flags( d );
            return;

        case MEDIT_AFF_FLAGS:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;
                if ( ( number > 0 ) || ( number < 31 ) ) {
                    number -= 1;
                    xTOGGLE_BIT( victim->affected_by, number );
                    olc_log( d, "%s the affect %s",
                             xIS_SET( victim->affected_by, number ) ? "Added" : "Removed",
                             a_flags[number] );
                }
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_actflag( arg1 );
                    if ( number > 0 ) {
                        xTOGGLE_BIT( victim->affected_by, number );
                        olc_log( d, "%s the affect %s",
                                 xIS_SET( victim->affected_by, number ) ? "Added" : "Removed",
                                 a_flags[number] );
                    }
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->affected_by = victim->affected_by;
            medit_disp_aff_flags( d );
            return;

/*-------------------------------------------------------------------*/
/*. Numerical responses .*/
        case MEDIT_HITPOINT:
            if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                write_to_descriptor( d,
                                     "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                     0 );
                break;
            }
            victim->max_hit = URANGE( 1, atoi( arg ), 32700 );
            olc_log( d, "Changed hitpoints to %d", victim->max_hit );
            break;

        case MEDIT_MANA:
            victim->max_mana = URANGE( 1, atoi( arg ), 30000 );
            olc_log( d, "Changed mana to %d", victim->max_mana );
            break;

        case MEDIT_MOVE:
            victim->max_move = URANGE( 1, atoi( arg ), 30000 );
            olc_log( d, "Changed moves to %d", victim->max_move );
            break;

        case MEDIT_PRACTICE:
            victim->practice = URANGE( 1, atoi( arg ), 300 );
            olc_log( d, "Changed practives to %d", victim->practice );
            break;

        case MEDIT_PASSWORD:
            if ( get_trust( d->character ) < LEVEL_AJ_SGT )
                break;
            if ( strlen( arg ) < 5 ) {
                send_to_char( "Password too short, try again: ", d->character );
                return;
            }
            pwdnew = crypt( arg, victim->name );
            for ( p = pwdnew; *p != '\0'; p++ ) {
                if ( *p == '~' ) {
                    send_to_char( "Unacceptable choice, try again: ", d->character );
                    return;
                }
            }
            DISPOSE( victim->pcdata->pwd );
            victim->pcdata->pwd = STRALLOC( pwdnew );
            if ( xIS_SET( sysdata.save_flags, SV_PASSCHG ) )
                save_char_obj( victim );
            olc_log( d, "Modified password" );
            break;

        case MEDIT_SAV1:
            victim->saving_poison_death = URANGE( -30, atoi( arg ), 30 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->saving_poison_death = victim->saving_poison_death;
            medit_disp_saving_menu( d );
            olc_log( d, "Changed save_poison_death to %d", victim->saving_poison_death );
            return;

        case MEDIT_SAV2:
            victim->saving_wand = URANGE( -30, atoi( arg ), 30 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->saving_wand = victim->saving_wand;
            medit_disp_saving_menu( d );
            olc_log( d, "Changed save_wand to %d", victim->saving_wand );
            return;

        case MEDIT_SAV3:
            victim->saving_para_petri = URANGE( -30, atoi( arg ), 30 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->saving_para_petri = victim->saving_para_petri;
            medit_disp_saving_menu( d );
            olc_log( d, "Changed save_paralysis_petrification to %d", victim->saving_para_petri );
            return;

        case MEDIT_SAV4:
            victim->saving_breath = URANGE( -30, atoi( arg ), 30 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->saving_breath = victim->saving_breath;
            medit_disp_saving_menu( d );
            olc_log( d, "Changed save_breath to %d", victim->saving_breath );
            return;

        case MEDIT_SAV5:
            victim->saving_spell_staff = URANGE( -30, atoi( arg ), 30 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->saving_spell_staff = victim->saving_spell_staff;
            medit_disp_saving_menu( d );
            olc_log( d, "Changed save_spell_staff to %d", victim->saving_spell_staff );
            return;

        case MEDIT_STRENGTH:
            victim->perm_str = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_str = victim->perm_str;
            olc_log( d, "Changed strength to %d", victim->perm_str );
            break;

        case MEDIT_INTELLIGENCE:
            victim->perm_int = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_int = victim->perm_int;
            olc_log( d, "Changed intelligence to %d", victim->perm_int );
            break;

        case MEDIT_WISDOM:
            victim->perm_wis = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_wis = victim->perm_wis;
            olc_log( d, "Changed victim wisdom to %d", victim->perm_wis );
            break;

        case MEDIT_DEXTERITY:
            victim->perm_dex = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_dex = victim->perm_dex;
            olc_log( d, "Changed dexterity to %d", victim->perm_dex );
            break;

        case MEDIT_CONSTITUTION:
            victim->perm_con = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_con = victim->perm_con;
            olc_log( d, "Changed constitution to %d", victim->perm_con );
            break;

        case MEDIT_CHARISMA:
            victim->perm_cha = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_cha = victim->perm_cha;
            olc_log( d, "Changed charisma to %d", victim->perm_cha );
            break;

        case MEDIT_LUCK:
            victim->perm_lck = URANGE( minattr, atoi( arg ), maxattr );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->perm_lck = victim->perm_lck;
            olc_log( d, "Changed luck to %d", victim->perm_lck );
            break;

        case MEDIT_SEX:
            victim->sex = URANGE( 0, atoi( arg ), 2 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->sex = victim->sex;
            olc_log( d, "Changed sex to %s",
                     victim->sex == 1 ? "Male" : victim->sex == 2 ? "Female" : "Neutral" );
            break;

        case MEDIT_HITROLL:
            if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                write_to_buffer( d,
                                 "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                 0 );
                break;
            }
            victim->hitroll = URANGE( 0, atoi( arg ), 200 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->hitroll = victim->hitroll;
            olc_log( d, "Changed hitroll to %d", victim->hitroll );
            break;

        case MEDIT_DAMROLL:
            if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                write_to_buffer( d,
                                 "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                 0 );
                break;
            }
            victim->damroll = URANGE( 0, atoi( arg ), 200 );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->damroll = victim->damroll;
            olc_log( d, "Changed damroll to %d", victim->damroll );
            break;

        case MEDIT_NUMATTACKS:
            if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                write_to_buffer( d,
                                 "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                 0 );
                break;
            }
            if ( IS_NPC( victim ) )
                victim->numattacks = URANGE( 0, atoi( arg ), 20 );
            olc_log( d, "Changed number of attacks to %d", victim->numattacks );
            break;

        case MEDIT_COLOR:
            if ( IS_NPC( victim ) )
                victim->color = URANGE( 0, atoi( arg ), 14 );
            olc_log( d, "Changed color to %d", victim->color );
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->color = victim->color;
            break;

        case MEDIT_AC:
            if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                write_to_buffer( d,
                                 "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                 0 );
                break;
            }
            victim->armor = URANGE( -300, atoi( arg ), 300 );
            olc_log( d, "Changed armor to %d", victim->armor );
            break;

        case MEDIT_GOLD:
            victim->gold = UMAX( 0, atoi( arg ) );
            olc_log( d, "Changed gold to %d", victim->gold );
            break;

        case MEDIT_POS:
            victim->position = URANGE( 0, atoi( arg ), POS_STANDING );
            olc_log( d, "Changed position to %d", victim->position );
            break;

        case MEDIT_DEFAULT_POS:
            victim->defposition = URANGE( 0, atoi( arg ), POS_STANDING );
            olc_log( d, "Changed default position to %d", victim->defposition );
            break;

        case MEDIT_MENTALSTATE:
            victim->mental_state = URANGE( -100, atoi( arg ), 100 );
            olc_log( d, "Changed mental state to %d", victim->mental_state );
            break;

        case MEDIT_EMOTIONAL:
            victim->emotional_state = URANGE( -100, atoi( arg ), 100 );
            olc_log( d, "Changed emotional state to %d", victim->emotional_state );
            break;

        case MEDIT_THIRST:
            victim->pcdata->condition[COND_THIRST] = URANGE( 0, atoi( arg ), 100 );
            olc_log( d, "Changed thirst to %d", victim->pcdata->condition[COND_THIRST] );
            break;

        case MEDIT_FULL:
            victim->pcdata->condition[COND_FULL] = URANGE( 0, atoi( arg ), 100 );
            olc_log( d, "Changed hunger to %d", victim->pcdata->condition[COND_FULL] );
            break;

        case MEDIT_DRUNK:
            victim->pcdata->condition[COND_DRUNK] = URANGE( 0, atoi( arg ), 100 );
            olc_log( d, "Changed drunkness to %d", victim->pcdata->condition[COND_DRUNK] );
            break;

        case MEDIT_FAVOR:
            victim->pcdata->favor = URANGE( -2500, atoi( arg ), 2500 );
            olc_log( d, "Changed favor to %d", victim->pcdata->favor );
            break;

        case MEDIT_SAVE_MENU:
            number = atoi( arg );
            switch ( number ) {
                default:
                    send_to_char( "Invalid saving throw, try again: ", d->character );
                    return;
                case 0:
                    break;
                case 1:
                    OLC_MODE( d ) = MEDIT_SAV1;
                    send_to_char( "\r\nEnter throw (-30 to 30): ", d->character );
                    return;
                case 2:
                    OLC_MODE( d ) = MEDIT_SAV2;
                    send_to_char( "\r\nEnter throw (-30 to 30): ", d->character );
                    return;
                case 3:
                    OLC_MODE( d ) = MEDIT_SAV3;
                    send_to_char( "\r\nEnter throw (-30 to 30): ", d->character );
                    return;
                case 4:
                    OLC_MODE( d ) = MEDIT_SAV4;
                    send_to_char( "\r\nEnter throw (-30 to 30): ", d->character );
                    return;
                case 5:
                    OLC_MODE( d ) = MEDIT_SAV5;
                    send_to_char( "\r\nEnter throw (-30 to 30): ", d->character );
                    return;
            }
            /*
             * If we reach here, we are going back to the main menu 
             */
            break;

        case MEDIT_CLASS:
            number = atoi( arg );
            if ( IS_NPC( victim ) ) {
                victim->Class = URANGE( 0, number, MAX_NPC_CLASS - 1 );
                if ( xIS_SET( victim->act, ACT_PROTOTYPE ) )
                    victim->pIndexData->Class = victim->Class;
                break;
            }
            victim->Class = URANGE( 0, number, MAX_CLASS );
            olc_log( d, "Changed class to %s", npc_class[victim->Class] );
            break;

        case MEDIT_RACE:
            number = atoi( arg );
            if ( IS_NPC( victim ) ) {
                victim->race = URANGE( 0, number, MAX_NPC_RACE - 1 );
                if ( xIS_SET( victim->act, ACT_PROTOTYPE ) )
                    victim->pIndexData->race = victim->race;
                break;
            }
            victim->race = URANGE( 0, number, MAX_RACE - 1 );
            olc_log( d, "Changed race to %s", npc_race[victim->race] );
            break;

        case MEDIT_PARTS:
            number = atoi( arg );
            if ( number < 0 || number > MAX_PARTS ) {
                send_to_char( "Invalid part, try again: ", d->character );
                return;
            }
            else {
                if ( !is_number( arg ) )
                    break;
                else {
                    number -= 1;
                    switch ( number ) {
                        case 0:
                            number = PART_HEAD;
                            break;
                        case 1:
                            number = PART_ARMS;
                            break;
                        case 2:
                            number = PART_LEGS;
                            break;
                        case 3:
                            number = PART_HEART;
                            break;
                        case 4:
                            number = PART_BRAINS;
                            break;
                        case 5:
                            number = PART_GUTS;
                            break;
                        case 6:
                            number = PART_HANDS;
                            break;
                        case 7:
                            number = PART_FEET;
                            break;
                        case 8:
                            number = PART_FINGERS;
                            break;
                        case 9:
                            number = PART_EAR;
                            break;
                        case 10:
                            number = PART_EYE;
                            break;
                        case 11:
                            number = PART_LONG_TONGUE;
                            break;
                        case 12:
                            number = PART_EYESTALKS;
                            break;
                        case 13:
                            number = PART_TENTACLES;
                            break;
                        case 14:
                            number = PART_FINS;
                            break;
                        case 15:
                            number = PART_WINGS;
                            break;
                        case 16:
                            number = PART_TAIL;
                            break;
                        case 17:
                            number = PART_SCALES;
                            break;
                        case 18:
                            number = PART_CLAWS;
                            break;
                        case 19:
                            number = PART_FANGS;
                            break;
                        case 20:
                            number = PART_HORNS;
                            break;
                        case 21:
                            number = PART_TUSKS;
                            break;
                        case 22:
                            number = PART_TAILATTACK;
                            break;
                        case 23:
                            number = PART_SHARPSCALES;
                            break;
                        case 24:
                            number = PART_BEAK;
                            break;
                        case 25:
                            number = PART_HAUNCH;
                            break;
                        case 26:
                            number = PART_HOOVES;
                            break;
                        case 27:
                            number = PART_PAWS;
                            break;
                        case 28:
                            number = PART_FORELEGS;
                            break;
                        case 29:
                            number = PART_FEATHERS;
                            break;
                    }

                    xTOGGLE_BIT( victim->xflags, number );
                }
                if ( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
                    victim->pIndexData->xflags = victim->xflags;
            }
            medit_disp_parts( d );
/*
         olc_log( d, "%s the body part %s", xIS_SET( victim->xflags, 1 << ( number - 1 ) ) ? "Added" : "Removed",
                  part_flags[number] ); */
            return;

        case MEDIT_ATTACK:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;

                number -= 1;                           /* offset */
                if ( number < 0 || number > MAX_ATTACK_TYPE + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                else
                    xTOGGLE_BIT( victim->attacks, number );
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_attackflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                    xTOGGLE_BIT( victim->attacks, number );
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->attacks = victim->attacks;
            medit_disp_attack_menu( d );
            olc_log( d, "%s the attack %s",
                     xIS_SET( victim->attacks, number ) ? "Added" : "Removed",
                     attack_flags[number] );
            return;

        case MEDIT_DEFENSE:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;

                number -= 1;                           /* offset */
                if ( number < 0 || number > MAX_DEFENSE_TYPE + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                else
                    xTOGGLE_BIT( victim->defenses, number );
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_defenseflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                    xTOGGLE_BIT( victim->defenses, number );
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->defenses = victim->defenses;
            medit_disp_defense_menu( d );
            olc_log( d, "%s the attack %s",
                     xIS_SET( victim->defenses, number ) ? "Added" : "Removed",
                     defense_flags[number] );
            return;

        case MEDIT_LEVEL:
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->level = atoi( arg );
            else {
                victim->level = atoi( arg );
            }
            victim->numattacks = set_num_attacks( victim->level );
            victim->armor = set_armor_class( victim->level );
            victim->max_hit = set_hp( victim->level );
            victim->hitplus = set_hp( victim->level );
            restore_char( victim );
            victim->hitroll = set_hitroll( victim->level );
            victim->damroll = set_damroll( victim->level );
            olc_log( d, "Changed level to %d", victim->level );
            do_savearea( d->character, ( char * ) "" );
            send_to_char
                ( "Now establishing Hitpoints, ArmorClass, Hitroll/Damroll and NumAttacks.\r\n",
                  d->character );
            break;

        case MEDIT_ALIGNMENT:
            victim->alignment = URANGE( -1000, atoi( arg ), 1000 );
            olc_log( d, "Changed alignment to %d", victim->alignment );
            break;

        case MEDIT_RESISTANT:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;

                number -= 1;                           /* offset */
                if ( number < 0 || number > MAX_RIS_FLAG + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                TOGGLE_BIT( victim->resistant, 1 << number );
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_risflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                    TOGGLE_BIT( victim->resistant, 1 << number );
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->resistant = victim->resistant;
            medit_disp_ris( d );
            olc_log( d, "%s the resistant %s",
                     IS_SET( victim->resistant, 1 << number ) ? "Added" : "Removed",
                     ris_flags[number] );
            return;

        case MEDIT_IMMUNE:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;

                number -= 1;
                if ( number < 0 || number > MAX_RIS_FLAG + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                TOGGLE_BIT( victim->immune, 1 << number );
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_risflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                    TOGGLE_BIT( victim->immune, 1 << number );
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->immune = victim->immune;

            medit_disp_ris( d );
            olc_log( d, "%s the immune %s",
                     IS_SET( victim->immune, 1 << number ) ? "Added" : "Removed",
                     ris_flags[number] );
            return;

        case MEDIT_SUSCEPTIBLE:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;

                number -= 1;
                if ( number < 0 || number > MAX_RIS_FLAG + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                TOGGLE_BIT( victim->susceptible, 1 << number );
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_risflag( arg1 );
                    if ( number < 0 ) {
                        send_to_char( "Invalid flag, try again: ", d->character );
                        return;
                    }
                    TOGGLE_BIT( victim->susceptible, 1 << number );
                }
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->susceptible = victim->susceptible;
            medit_disp_ris( d );
            olc_log( d, "%s the suscept %s",
                     IS_SET( victim->susceptible, 1 << number ) ? "Added" : "Removed",
                     ris_flags[number] );
            return;

        case MEDIT_SPEC:
            number = atoi( arg );
            switch ( number ) {
                case 0:
                    victim->spec_fun = NULL;
                    break;
                case 1:
                    victim->spec_fun = spec_lookup( "spec_breath_any" );
                    break;
                case 2:
                    victim->spec_fun = spec_lookup( "spec_breath_acid" );
                    break;
                case 3:
                    victim->spec_fun = spec_lookup( "spec_breath_fire" );
                    break;
                case 4:
                    victim->spec_fun = spec_lookup( "spec_breath_frost" );
                    break;
                case 5:
                    victim->spec_fun = spec_lookup( "spec_breath_gas" );
                    break;
                case 6:
                    victim->spec_fun = spec_lookup( "spec_breath_lightning" );
                    break;
                case 7:
                    victim->spec_fun = spec_lookup( "spec_cast_adept" );
                    break;
                case 8:
                    victim->spec_fun = spec_lookup( "spec_cast_cleric" );
                    break;
                case 9:
                    victim->spec_fun = spec_lookup( "spec_cast_mage" );
                    break;
                case 10:
                    victim->spec_fun = spec_lookup( "spec_cast_undead" );
                    break;
                case 11:
                    victim->spec_fun = spec_lookup( "spec_executioner" );
                    break;
                case 12:
                    victim->spec_fun = spec_lookup( "spec_fido" );
                    break;
                case 13:
                    victim->spec_fun = spec_lookup( "spec_guard" );
                    break;
                case 14:
                    victim->spec_fun = spec_lookup( "spec_janitor" );
                    break;
                case 15:
                    victim->spec_fun = spec_lookup( "spec_poison" );
                    break;
                case 16:
                    victim->spec_fun = spec_lookup( "spec_thief" );
                    break;
            }
            if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                victim->pIndexData->spec_fun = victim->spec_fun;
            olc_log( d, "Changes spec_func to %s", lookup_spec( victim->spec_fun ) );
            break;

        case MEDIT_DEITY:
            number = atoi( arg );
            if ( number < 0 || number > olc_top_deity + 1 ) {
                send_to_char( "Invalid deity, try again: ", d->character );
                return;
            }
            if ( number == 0 ) {
                STRFREE( victim->pcdata->deity_name );
                victim->pcdata->deity_name = STRALLOC( "" );
                victim->pcdata->deity = NULL;
                break;
            }
            number -= 1;
            deity = get_deity( olc_deity_list[number] );
            if ( !deity ) {
                bug( "Unknown deity linked into olc_deity_list." );
                break;
            }
            STRFREE( victim->pcdata->deity_name );
            victim->pcdata->deity_name = QUICKLINK( deity->name );
            victim->pcdata->deity = deity;
            olc_log( d, "Deity changed to %s", deity->name );
            break;

        case MEDIT_CLAN:
            if ( get_trust( d->character ) < LEVEL_IMMORTAL )
                break;
            number = atoi( arg );
            if ( number < 0 || number > olc_top_order + 1 ) {
                send_to_char( "Invalid choice, try again: ", d->character );
                return;
            }
            if ( number == 0 ) {
                if ( !IS_IMMORTAL( victim ) ) {
                    --victim->pcdata->clan->members;
                    save_clan( victim->pcdata->clan );
                }
                STRFREE( victim->pcdata->clan_name );
                victim->pcdata->clan_name = STRALLOC( "" );
                victim->pcdata->clan = NULL;
                break;
            }
            clan = get_clan( olc_clan_list[number - 1] );
            if ( !clan ) {
                bug( "Non-existant clan linked into olc_clan_list." );
                break;
            }
            if ( victim->pcdata->clan != NULL && !IS_IMMORTAL( victim ) ) {
                --victim->pcdata->clan->members;
                save_clan( victim->pcdata->clan );
            }
            STRFREE( victim->pcdata->clan_name );
            victim->pcdata->clan_name = QUICKLINK( clan->name );
            victim->pcdata->clan = clan;
            if ( !IS_IMMORTAL( victim ) ) {
                ++victim->pcdata->clan->members;
                save_clan( victim->pcdata->clan );
            }
            olc_log( d, "Clan changed to %s", clan->name );
            break;

        case MEDIT_COUNCIL:
            if ( get_trust( d->character ) < LEVEL_IMMORTAL )
                break;
            number = atoi( arg );
            if ( number < 0 || number > olc_top_council ) {
                send_to_char( "Invalid council, try again: ", d->character );
                return;
            }
            if ( number == 0 ) {
                STRFREE( victim->pcdata->council_name );
                victim->pcdata->council_name = STRALLOC( "" );
                victim->pcdata->council = NULL;
                break;
            }
            number -= 1;                               /* Offset cause 0 is first element 
                                                        * but 0 is None, soo */
            council = get_council( olc_council_list[number] );
            if ( !council ) {
                bug( "Unknown council linked into olc_council_list." );
                break;
            }
            STRFREE( victim->pcdata->council_name );
            victim->pcdata->council_name = QUICKLINK( council->name );
            victim->pcdata->council = council;
            olc_log( d, "Council changed to %s", council->name );
            break;

/*-------------------------------------------------------------------*/
        default:
            /*
             * . We should never get here .
             */
            bug( "OLC: medit_parse(): Reached default case!" );
            cleanup_olc( d );
            return;;
    }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_CHANGE as a 'has changed' flag .*/

    OLC_CHANGE( d ) = TRUE;
    medit_disp_menu( d );
}

/*. End of medit_parse() .*/
