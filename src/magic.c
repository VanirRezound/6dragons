/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Spell handling module                                                 *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#ifdef sun
#include <strings.h>
#endif
#include "h/mud.h"
#include "h/clans.h"
#include "h/polymorph.h"
#include "h/damage.h"

#define MIN_DISTANCE 1

/*
 * External functions.
 */
void weapon_type        args( ( CHAR_DATA *ch, int value ) );
extern bool             fBootDb;

/*
 * Local functions.
 */
void say_spell          args( ( CHAR_DATA *ch, int sn ) );

/*
CHAR_DATA *make_poly_mob args((CHAR_DATA *ch, int vnum));
*/
ch_ret spell_affect     args( ( int sn, int level, CHAR_DATA *ch, void *vo ) );
ch_ret spell_affectchar args( ( int sn, int level, CHAR_DATA *ch, void *vo ) );
int                     dispel_casting( AFFECT_DATA *paf, CHAR_DATA *ch, CHAR_DATA *victim,
                                        int affect, bool dispel );
bool                    can_charm( CHAR_DATA *ch );
void                    to_channel( const char *argument, const char *xchannel, int level );

/*
 * Lookup a skill by name, only stopping at skills the player has.
 */
int ch_slookup( CHAR_DATA *ch, const char *name )
{
    int                     sn;

    if ( IS_NPC( ch ) )
        return skill_lookup( name );
    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        // Modified to support multi-classing. -Taon. 
        if ( ch->pcdata->learned[sn] > 0 && ch->level >= skill_table[sn]->skill_level[ch->Class]
             && LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
             && !str_prefix( name, skill_table[sn]->name ) || ch->pcdata->learned[sn] > 0
             && ch->level >= skill_table[sn]->skill_level[ch->secondclass]
             && LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
             && !str_prefix( name, skill_table[sn]->name ) || ch->pcdata->learned[sn] > 0
             && ch->level >= skill_table[sn]->skill_level[ch->thirdclass]
             && LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
             && !str_prefix( name, skill_table[sn]->name ) )
            return sn;
    }

    return -1;
}

/*
 * Lookup an herb by name.
 */
int herb_lookup( const char *name )
{
    int                     sn;

    for ( sn = 0; sn < top_herb; sn++ ) {
        if ( !herb_table[sn] || !herb_table[sn]->name )
            return -1;
        if ( LOWER( name[0] ) == LOWER( herb_table[sn]->name[0] )
             && !str_prefix( name, herb_table[sn]->name ) )
            return sn;
    }
    return -1;
}

extern const char      *wear_bit_name( int wear_flags );

/* Return ascii name of wear flags vector. - Added by Samson 2-8-98 */
const char             *wear_bit_name( int wear_flags )
{
    static char             buf[512];

    buf[0] = '\0';
    if ( wear_flags & ITEM_TAKE )
        mudstrlcat( buf, " take", 512 );
    if ( wear_flags & ITEM_WEAR_FINGER )
        mudstrlcat( buf, " finger", 512 );
    if ( wear_flags & ITEM_WEAR_NECK )
        mudstrlcat( buf, " neck", 512 );
    if ( wear_flags & ITEM_WEAR_BODY )
        mudstrlcat( buf, " body", 512 );
    if ( wear_flags & ITEM_WEAR_HEAD )
        mudstrlcat( buf, " head", 512 );
    if ( wear_flags & ITEM_WEAR_LEGS )
        mudstrlcat( buf, " legs", 512 );
    if ( wear_flags & ITEM_WEAR_FEET )
        mudstrlcat( buf, " feet", 512 );
    if ( wear_flags & ITEM_WEAR_HANDS )
        mudstrlcat( buf, " hands", 512 );
    if ( wear_flags & ITEM_WEAR_ARMS )
        mudstrlcat( buf, " arms", 512 );
    if ( wear_flags & ITEM_WEAR_SHIELD )
        mudstrlcat( buf, " shield", 512 );
    if ( wear_flags & ITEM_WEAR_ABOUT )
        mudstrlcat( buf, " about", 512 );
    if ( wear_flags & ITEM_WEAR_WAIST )
        mudstrlcat( buf, " waist", 512 );
    if ( wear_flags & ITEM_WEAR_WRIST )
        mudstrlcat( buf, " wrist", 512 );
    if ( wear_flags & ITEM_WIELD )
        mudstrlcat( buf, " wield", 512 );
    if ( wear_flags & ITEM_HOLD )
        mudstrlcat( buf, " hold", 512 );
    if ( wear_flags & ITEM_DUAL_WIELD )
        mudstrlcat( buf, " dual-wield", 512 );
    if ( wear_flags & ITEM_WEAR_EARS )
        mudstrlcat( buf, " ears", 512 );
    if ( wear_flags & ITEM_WEAR_EYES )
        mudstrlcat( buf, " eyes", 512 );
    if ( wear_flags & ITEM_MISSILE_WIELD )
        mudstrlcat( buf, " missile", 512 );
    if ( wear_flags & ITEM_WEAR_BACK )
        mudstrlcat( buf, " back", 512 );
    if ( wear_flags & ITEM_WEAR_FACE )
        mudstrlcat( buf, " face", 512 );
    if ( wear_flags & ITEM_WEAR_ANKLE )
        mudstrlcat( buf, " ankle", 512 );
    return ( buf[0] != '\0' ) ? buf + 1 : "none";
}

/* don't remove, may look redundant, but is important */
ch_ret spell_notfound( int sn, int level, CHAR_DATA *ch, void *vo )
{
    send_to_char( "That's not a spell!\r\n", ch );
    return rNONE;
}

ch_ret spell_null( int sn, int level, CHAR_DATA *ch, void *vo )
{
    send_to_char( "That's not a spell!\r\n", ch );
    return rNONE;
}

/*
 * Lookup a personal skill
 * Unused for now.  In place to allow a player to have a custom spell/skill.
 * When this is put in make sure you put in cleanup code if you do any
 * sort of allocating memory in free_char --Shaddai
 */
int personal_lookup( CHAR_DATA *ch, const char *name )
{
    int                     sn;

    if ( !ch->pcdata )
        return -1;
    for ( sn = 0; sn < MAX_PERSONAL; sn++ ) {
        if ( !ch->pcdata->special_skills[sn] || !ch->pcdata->special_skills[sn]->name )
            return -1;
        if ( LOWER( name[0] ) == LOWER( ch->pcdata->special_skills[sn]->name[0] )
             && !str_prefix( name, ch->pcdata->special_skills[sn]->name ) )
            return sn;
    }
    return -1;
}

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int                     sn;

    if ( ( sn = bsearch_skill_exact( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
        if ( ( sn = bsearch_skill_exact( name, gsn_first_skill, gsn_first_weapon - 1 ) ) == -1 )
            if ( ( sn =
                   bsearch_skill_exact( name, gsn_first_weapon, gsn_first_tongue - 1 ) ) == -1 )
                if ( ( sn =
                       bsearch_skill_exact( name, gsn_first_tongue, gsn_first_song - 1 ) ) == -1 )
                    if ( ( sn =
                           bsearch_skill_exact( name, gsn_first_song, gsn_top_sn - 1 ) ) == -1 )
                        if ( ( sn =
                               bsearch_skill_prefix( name, gsn_first_spell,
                                                     gsn_first_skill - 1 ) ) == -1 )
                            if ( ( sn =
                                   bsearch_skill_prefix( name, gsn_first_skill,
                                                         gsn_first_weapon - 1 ) ) == -1 )
                                if ( ( sn =
                                       bsearch_skill_prefix( name, gsn_first_weapon,
                                                             gsn_first_tongue - 1 ) ) == -1 )
                                    if ( ( sn =
                                           bsearch_skill_prefix( name, gsn_first_tongue,
                                                                 gsn_first_song - 1 ) ) == -1 )
                                        if ( ( sn =
                                               bsearch_skill_prefix( name, gsn_first_song,
                                                                     gsn_top_sn - 1 ) ) == -1
                                             && gsn_top_sn < top_sn ) {
                                            for ( sn = gsn_top_sn; sn < top_sn; ++sn ) {
                                                if ( !skill_table[sn] || !skill_table[sn]->name )
                                                    return -1;
                                                if ( LOWER( name[0] ) ==
                                                     LOWER( skill_table[sn]->name[0] )
                                                     && !str_prefix( name, skill_table[sn]->name ) )
                                                    return sn;
                                            }
                                            return -1;
                                        }
    return sn;
}

/*
 * Return a skilltype pointer based on sn   -Thoric
 * Returns NULL if bad, unused or personal sn.
 */

SKILLTYPE              *get_skilltype( int sn )
{
    if ( sn >= TYPE_PERSONAL )
        return NULL;
    if ( sn >= TYPE_HERB )
        return IS_VALID_HERB( sn - TYPE_HERB ) ? herb_table[sn - TYPE_HERB] : NULL;
    if ( sn >= TYPE_HIT )
        return NULL;
    return IS_VALID_SN( sn ) ? skill_table[sn] : NULL;
}

/*
 * Perform a binary search on a section of the skill table  -Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for prefix matches
 */
int bsearch_skill_prefix( const char *name, int first, int top )
{
    int                     sn;

    for ( ;; ) {
        sn = ( first + top ) >> 1;
        if ( !IS_VALID_SN( sn ) )
            return -1;
        if ( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
             && !str_prefix( name, skill_table[sn]->name ) )
            return sn;
        if ( first >= top )
            return -1;
        if ( strcmp( name, skill_table[sn]->name ) < 1 )
            top = sn - 1;
        else
            first = sn + 1;
    }
    return -1;
}

/*
 * Perform a binary search on a section of the skill table  -Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for exact matches only
 */
int bsearch_skill_exact( const char *name, int first, int top )
{
    int                     sn;

    for ( ;; ) {
        sn = ( first + top ) >> 1;
        if ( !IS_VALID_SN( sn ) )
            return -1;
        if ( !str_cmp( name, skill_table[sn]->name ) )
            return sn;
        if ( first >= top )
            return -1;
        if ( strcmp( name, skill_table[sn]->name ) < 1 )
            top = sn - 1;
        else
            first = sn + 1;
    }
//  return -1;
}

/*
 * Perform a binary search on a section of the skill table  -Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check exact match first, then a prefix match
 */
int bsearch_skill( const char *name, int first, int top )
{
    int                     sn = bsearch_skill_exact( name, first, top );

    return ( sn == -1 ) ? bsearch_skill_prefix( name, first, top ) : sn;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows     -Thoric
 */
int ch_bsearch_skill_prefix( CHAR_DATA *ch, const char *name, int first, int top )
{
    int                     sn;

    for ( ;; ) {
        sn = ( first + top ) >> 1;

        if ( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( name, skill_table[sn]->name ) && ch->pcdata->learned[sn] > 0 &&  /* (ch->level 
                                                                                                                                                     * >= 
                                                                                                                                                     * get_maxskill(ch, 
                                                                                                                                                     * sn, 
                                                                                                                                                     * TRUE) 
                                                                                                                                                     */ CAN_LEARN( ch, sn, TRUE ) )
            return sn;

        if ( first >= top )
            return -1;
        if ( strcmp( name, skill_table[sn]->name ) < 1 )
            top = sn - 1;
        else
            first = sn + 1;
    }
    return -1;
}

int ch_bsearch_skill_exact( CHAR_DATA *ch, const char *name, int first, int top )
{
    int                     sn;

    for ( ;; ) {
        sn = ( first + top ) >> 1;

        if ( !str_cmp( name, skill_table[sn]->name ) && ch->pcdata->learned[sn] > 0 &&  /* (ch->level 
                                                                                         * >= 
                                                                                         * get_maxskill(ch, 
                                                                                         * sn, 
                                                                                         * TRUE) 
                                                                                         */ CAN_LEARN( ch, sn, TRUE ) )
            return sn;

        if ( first >= top )
            return -1;
        if ( strcmp( name, skill_table[sn]->name ) < 1 )
            top = sn - 1;
        else
            first = sn + 1;
    }
    return -1;
}

int ch_bsearch_skill( CHAR_DATA *ch, const char *name, int first, int top )
{
    int                     sn = ch_bsearch_skill_exact( ch, name, first, top );

    return ( sn == -1 ) ? ch_bsearch_skill_prefix( ch, name, first, top ) : sn;
}

int find_spell( CHAR_DATA *ch, const char *name, bool know )
{
    if ( IS_NPC( ch ) || !know )
        return bsearch_skill( name, gsn_first_spell, gsn_first_skill - 1 );
    else
        return ch_bsearch_skill( ch, name, gsn_first_spell, gsn_first_skill - 1 );
}

int find_skill( CHAR_DATA *ch, const char *name, bool know )
{
    if ( IS_NPC( ch ) || !know )
        return bsearch_skill( name, gsn_first_skill, gsn_first_weapon - 1 );
    else
        return ch_bsearch_skill( ch, name, gsn_first_skill, gsn_first_weapon - 1 );
}

int find_weapon( CHAR_DATA *ch, const char *name, bool know )
{
    if ( IS_NPC( ch ) || !know )
        return bsearch_skill( name, gsn_first_weapon, gsn_first_tongue - 1 );
    else
        return ch_bsearch_skill( ch, name, gsn_first_weapon, gsn_first_tongue - 1 );
}

int find_tongue( CHAR_DATA *ch, const char *name, bool know )
{
    if ( IS_NPC( ch ) || !know )
        return bsearch_skill( name, gsn_first_tongue, gsn_first_song - 1 );
    else
        return ch_bsearch_skill( ch, name, gsn_first_tongue, gsn_first_song - 1 );
}

/*
int find_trade(CHAR_DATA * ch, const char *name, bool know)
{
  if(IS_NPC(ch) || !know)
    return bsearch_skill(name, gsn_first_trade, gsn_top_sn - 1);
  else
    return ch_bsearch_skill(ch, name, gsn_first_trade, gsn_top_sn - 1);
}
*/
/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
/*    extern bool fBootDb;  */
    int                     sn;

    if ( slot <= 0 )
        return -1;

    for ( sn = 0; sn < top_sn; sn++ )
        if ( slot == skill_table[sn]->slot )
            return sn;

    if ( fBootDb ) {
        bug( "Slot_lookup: bad slot %d.", slot );
        abort(  );
    }

    return -1;
}

/*
 * Handler to tell the victim which spell is being affected.
 * Shaddai
 */
int dispel_casting( AFFECT_DATA *paf, CHAR_DATA *ch, CHAR_DATA *victim, int affect, bool dispel )
{
    char                    buf[MSL],
                           *spell;
    SKILLTYPE              *sktmp;
    bool                    is_mage = FALSE,
        has_detect = FALSE;
    EXT_BV                  ext_bv = meb( affect );

    if ( IS_NPC( ch ) || ch->Class == CLASS_MAGE || ch->secondclass == CLASS_MAGE
         || ch->thirdclass == CLASS_MAGE )
        is_mage = TRUE;
    if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) )
        has_detect = TRUE;

    if ( !paf && affect == -1 )
        return 0;

    if ( paf ) {
        if ( ( sktmp = get_skilltype( paf->type ) ) == NULL )
            return 0;
        spell = sktmp->name;
    }
    else
        spell = ext_flag_string( &ext_bv, a_flags );

    set_char_color( AT_MAGIC, ch );
    set_char_color( AT_HITME, victim );
    if ( !can_see( ch, victim ) )
        mudstrlcpy( buf, "Someone", MSL );
    else {
        mudstrlcpy( buf, ( IS_NPC( victim ) ? victim->short_descr : victim->name ), MSL );
        buf[0] = toupper( buf[0] );
    }

    if ( dispel ) {
        ch_printf( victim, "Your %s vanishes.\r\n", spell );
        act_printf( AT_MAGIC, victim, NULL, NULL, TO_ROOM, "$n's %s vanishes.", spell );
    }
    else
        ch_printf( ch, "%s's %s wavers but holds.\r\n", buf, spell );
    return 1;
}

/* Fancy message handling for a successful casting  -Thoric */
void successful_casting( SKILLTYPE * skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    short                   chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
    short                   chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
    short                   chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

    if ( skill->target != TAR_CHAR_OFFENSIVE ) {
        chit = chitroom;
        chitme = chitroom;
    }
    if ( IS_AFFECTED( ch, AFF_SILENCE ) ) {
        send_to_char( "You try to cast a spell, but cannot speak.\r\n", ch );
        return;
    }
    if ( ch && ch != victim ) {
        if ( skill->hit_char && skill->hit_char[0] != '\0' )
            act( AT_MAGIC, skill->hit_char, ch, obj, victim, TO_CHAR );
        else if ( skill->type == SKILL_SPELL )
            act( AT_MAGIC, "You utter some arcane words.", ch, NULL, NULL, TO_CHAR );
    }
    if ( ch && skill->hit_room && skill->hit_room[0] != '\0' )
        act( AT_MAGIC, skill->hit_room, ch, obj, victim, TO_NOTVICT );
    if ( ch && victim && skill->hit_vict && skill->hit_vict[0] != '\0' ) {
        if ( str_cmp( skill->hit_vict, SPELL_SILENT_MARKER ) ) {
            if ( ch != victim )
                act( AT_MAGIC, skill->hit_vict, ch, obj, victim, TO_VICT );
            else
                act( AT_MAGIC, skill->hit_vict, ch, obj, victim, TO_CHAR );
        }
    }
    else if ( ch && ch == victim && skill->type == SKILL_SPELL )
        act( AT_MAGIC, "You utter some arcane words.", ch, NULL, NULL, TO_CHAR );
}

/* Fancy message handling for a failed casting -Thoric */
void failed_casting( SKILLTYPE * skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    short                   chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
    short                   chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
    short                   chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

    if ( skill->target != TAR_CHAR_OFFENSIVE ) {
        chit = chitroom;
        chitme = chitroom;
    }
    if ( IS_AFFECTED( ch, AFF_SILENCE ) ) {
        send_to_char( "You try to cast a spell, but cannot speak.\r\n", ch );
        return;
    }
    if ( ch && ch != victim ) {
        if ( skill->miss_char && skill->miss_char[0] != '\0' )
            act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
        else if ( skill->type == SKILL_SPELL )
            act( chit, "You failed.", ch, NULL, NULL, TO_CHAR );
    }
    if ( ch && skill->miss_room && skill->miss_room[0] != '\0'
         && str_cmp( skill->miss_room, "supress" ) )
        act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
    if ( ch && victim && skill->miss_vict && skill->miss_vict[0] != '\0' ) {
        if ( ch != victim )
            act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
        else
            act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
    }
    else if ( ch && ch == victim ) {
        if ( skill->miss_char && skill->miss_char[0] != '\0' )
            act( chitme, skill->miss_char, ch, obj, victim, TO_CHAR );
        else if ( skill->type == SKILL_SPELL )
            act( chitme, "You failed.", ch, NULL, NULL, TO_CHAR );
    }
}

/* Fancy message handling for being immune to something  -Thoric */
void immune_casting( SKILLTYPE * skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    short                   chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
    short                   chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
    short                   chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

    if ( skill->target != TAR_CHAR_OFFENSIVE ) {
        chit = chitroom;
        chitme = chitroom;
    }
    if ( ch && ch != victim ) {
        if ( VLD_STR( skill->imm_char ) )
            act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
        else if ( VLD_STR( skill->miss_char ) )
            act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
        else if ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
            act( chit, "That appears to have no effect.", ch, NULL, NULL, TO_CHAR );
    }
    if ( ch && VLD_STR( skill->imm_room ) )
        act( chitroom, skill->imm_room, ch, obj, victim, TO_NOTVICT );
    else if ( ch && VLD_STR( skill->miss_room ) )
        act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
    if ( ch && victim && VLD_STR( skill->imm_vict ) ) {
        if ( ch != victim )
            act( chitme, skill->imm_vict, ch, obj, victim, TO_VICT );
        else
            act( chitme, skill->imm_vict, ch, obj, victim, TO_CHAR );
    }
    else if ( ch && victim && VLD_STR( skill->miss_vict ) ) {
        if ( ch != victim )
            act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
        else
            act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
    }
    else if ( ch && ch == victim ) {
        if ( VLD_STR( skill->imm_char ) )
            act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
        else if ( VLD_STR( skill->miss_char ) )
            act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
        else if ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
            act( chit, "That appears to have no affect.", ch, NULL, NULL, TO_CHAR );
    }
}

/* Utter mystical words for an sn. */
void say_spell( CHAR_DATA *ch, int sn )
{
    char                    buf[MSL],
                            buf2[MSL],
                           *pName;
    CHAR_DATA              *rch;
    int                     iSyl;
    int                     length;
    SKILLTYPE              *skill = get_skilltype( sn );

    struct syl_type
    {
        const char             *old;
        const char             *lnew;
    };

    static const struct syl_type syl_table[] = {
        {" ", " "}, {"ar", "abra"},
        {"au", "kada"}, {"bless", "fido"},
        {"blind", "nose"}, {"bur", "mosa"},
        {"cu", "judi"}, {"de", "oculo"},
        {"en", "unso"}, {"light", "dies"},
        {"lo", "hi"}, {"mor", "zak"},
        {"move", "sido"}, {"ness", "lacri"},
        {"ning", "illa"}, {"per", "duda"},
        {"polymorph", "iaddahs"}, {"ra", "gru"},
        {"re", "candus"}, {"son", "sabru"},
        {"tect", "infra"}, {"tri", "cula"},
        {"ven", "nofo"}, {"a", "a"},
        {"b", "b"}, {"c", "q"}, {"d", "e"},
        {"e", "z"}, {"f", "y"}, {"g", "o"}, {"h", "p"},
        {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
        {"m", "w"}, {"n", "i"}, {"o", "a"}, {"p", "s"},
        {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
        {"u", "j"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
        {"y", "l"}, {"z", "k"},
        {"", ""}
    };
    buf[0] = '\0';
    for ( pName = skill->name; *pName != '\0'; pName += length ) {
        for ( iSyl = 0; ( length = strlen( syl_table[iSyl].old ) ) != 0; iSyl++ ) {
            if ( !str_prefix( syl_table[iSyl].old, pName ) ) {
                mudstrlcat( buf, syl_table[iSyl].lnew, MSL );
                break;
            }
        }
        if ( length == 0 )
            length = 1;
    }
    snprintf( buf2, MSL, "$n utters the words, '%s'.", buf );
    snprintf( buf, MSL, "$n utters the words, '%s'.", skill->name );
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
        if ( rch != ch )
            act( AT_MAGIC, ch->Class == rch->Class ? buf : buf2, ch, NULL, rch, TO_VICT );
    }
    return;
}

/* Make adjustments to saving throw based in RIS  -Thoric */
int ris_save( CHAR_DATA *ch, int chance, int ris )
{
    short                   modifier;

    modifier = 10;
    if ( IS_IMMUNE( ch, ris ) )
        modifier -= 10;
    if ( IS_RESIS( ch, ris ) )
        modifier -= 2;
    if ( IS_SUSCEP( ch, ris ) ) {
        if ( IS_NPC( ch ) && IS_IMMUNE( ch, ris ) )
            modifier += 0;
        else
            modifier += 2;
    }
    if ( modifier <= 0 )
        return 1000;
    if ( modifier == 10 )
        return chance;
    return ( chance * modifier ) / 10;
}

/*            -Thoric
 * Fancy dice expression parsing complete with order of operations,
 * simple exponent support, dice support as well as a few extra
 * variables: L = level, H = hp, M = mana, V = move, S = str, X = dex
 *            I = int, W = wis, C = con, A = cha, U = luck, A = age
 *
 * Used for spell dice parsing, ie: 3d8+L-6
 *
 */
int rd_parse( CHAR_DATA *ch, int level, char *exp )
{
    unsigned int            x;
    int                     lop = 0,
        gop = 0,
        eop = 0;
    char                    operation;
    char                   *sexp[2];
    int                     total = 0;
    unsigned int            len = 0;

    /*
     * take care of nulls coming in 
     */
    if ( !exp || !strlen( exp ) )
        return 0;

    /*
     * get rid of brackets if they surround the entire expresion 
     */
    if ( ( *exp == '(' ) && exp[strlen( exp ) - 1] == ')' ) {
        exp[strlen( exp ) - 1] = '\0';
        exp++;
    }

    /*
     * check if the expresion is just a number 
     */
    len = strlen( exp );
    if ( len == 1 && isalpha( exp[0] ) ) {
        switch ( exp[0] ) {
            case 'L':
            case 'l':
                return level;
            case 'N':
            case 'n':
                return nominal;
            case 'O':
            case 'o':
                return low;
            case 'P':
            case 'p':
                return medium;
            case 'Q':
            case 'q':
                return mediumhigh;
            case 'R':
            case 'r':
                return high;
            case 'G':
            case 'g':
                return extrahigh;
            case 'T':
            case 't':
                return ludicrous;
            case 'Z':
            case 'z':
                return insane;
            case 'J':
            case 'j':
                return maximum;
            case 'H':
            case 'h':
                return ch->hit;
            case 'M':
            case 'm':
                return ch->mana;
            case 'V':
            case 'v':
                return ch->move;
            case 'S':
            case 's':
                return get_curr_str( ch );
            case 'I':
            case 'i':
                return get_curr_int( ch );
            case 'W':
            case 'w':
                return get_curr_wis( ch );
            case 'X':
            case 'x':
                return get_curr_dex( ch );
            case 'C':
            case 'c':
                return get_curr_con( ch );
            case 'A':
            case 'a':
                return get_curr_cha( ch );
            case 'U':
            case 'u':
                return get_curr_lck( ch );
            case 'Y':
            case 'y':
                return calculate_age( ch );
        }
    }

    for ( x = 0; x < len; ++x )
        if ( !isdigit( exp[x] ) && !isspace( exp[x] ) )
            break;
    if ( x == len )
        return atoi( exp );

    /*
     * break it into 2 parts 
     */
    for ( x = 0; x < strlen( exp ); ++x )
        switch ( exp[x] ) {
            case '^':
                if ( !total )
                    eop = x;
                break;
            case '-':
            case '+':
                if ( !total )
                    lop = x;
                break;
            case '*':
            case '/':
            case '%':
            case 'd':
            case 'D':
            case '<':
            case '>':
            case '{':
            case '}':
            case '=':
                if ( !total )
                    gop = x;
                break;
            case '(':
                ++total;
                break;
            case ')':
                --total;
                break;
        }
    if ( lop )
        x = lop;
    else if ( gop )
        x = gop;
    else
        x = eop;
    operation = exp[x];
    exp[x] = '\0';
    sexp[0] = exp;
    sexp[1] = ( char * ) ( exp + x + 1 );

    /*
     * work it out 
     */
    total = rd_parse( ch, level, sexp[0] );
    switch ( operation ) {
        case '-':
            total -= rd_parse( ch, level, sexp[1] );
            break;
        case '+':
            total += rd_parse( ch, level, sexp[1] );
            break;
        case '*':
            total *= rd_parse( ch, level, sexp[1] );
            break;
        case '/':
            total /= rd_parse( ch, level, sexp[1] );
            break;
        case '%':
            total %= rd_parse( ch, level, sexp[1] );
            break;
        case 'd':
        case 'D':
            total = dice( total, rd_parse( ch, level, sexp[1] ) );
            break;
        case '<':
            total = ( total < rd_parse( ch, level, sexp[1] ) );
            break;
        case '>':
            total = ( total > rd_parse( ch, level, sexp[1] ) );
            break;
        case '=':
            total = ( total == rd_parse( ch, level, sexp[1] ) );
            break;
        case '{':
            total = UMIN( total, rd_parse( ch, level, sexp[1] ) );
            break;
        case '}':
            total = UMAX( total, rd_parse( ch, level, sexp[1] ) );
            break;

        case '^':
            {
                unsigned int            y = rd_parse( ch, level, sexp[1] ),
                    z = total;

                for ( x = 1; x < y; ++x, z *= total );
                total = z;
                break;
            }
    }
    return total;
}

/* wrapper function so as not to destroy exp */
int dice_parse( CHAR_DATA *ch, int level, char *exp )
{
    char                    buf[MIL];

    mudstrlcpy( buf, exp, MIL );
    return rd_parse( ch, level, buf );
}

/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_poison_death( int level, CHAR_DATA *victim )
{
    int                     save;

    save = 50 + ( victim->level - level - victim->saving_poison_death ) * 5;
    save = URANGE( 5, save, 95 );
    return chance( victim, save );
}

bool saves_wands( int level, CHAR_DATA *victim )
{
    int                     save;

    if ( IS_IMMUNE( victim, RIS_MAGIC ) )
        return TRUE;

    save = 50 + ( victim->level - level - victim->saving_wand ) * 5;
    save = URANGE( 5, save, 95 );
    return chance( victim, save );
}

bool saves_para_petri( int level, CHAR_DATA *victim )
{
    int                     save;

    save = 50 + ( victim->level - level - victim->saving_para_petri ) * 5;
    save = URANGE( 5, save, 95 );
    return chance( victim, save );
}

bool saves_breath( int level, CHAR_DATA *victim )
{
    int                     save;

    save = 50 + ( victim->level - level - victim->saving_breath ) * 5;
    save = URANGE( 5, save, 95 );
    return chance( victim, save );
}

bool saves_spell_staff( int level, CHAR_DATA *victim )
{
    int                     save;

    if ( IS_IMMUNE( victim, RIS_MAGIC ) )
        return TRUE;

    if ( IS_NPC( victim ) && level > 10 )
        level -= 5;
    save = 50 + ( victim->level - level - victim->saving_spell_staff ) * 5;
    save = URANGE( 5, save, 95 );
    return chance( victim, save );
}

/*
 * Process the spell's required components, if any  -Thoric
 * -----------------------------------------------
 * T###  check for item of type ###
 * V#####  check for item of vnum #####
 * Kword  check for item with keyword 'word'
 * G#####  check if player has ##### amount of gold
 * H####  check if player has #### amount of hitpoints
 *
 * Special operators:
 * ! spell fails if player has this
 * + don't consume this component
 * @ decrease component's value[0], and extract if it reaches 0
 * # decrease component's value[1], and extract if it reaches 0
 * $ decrease component's value[2], and extract if it reaches 0
 * % decrease component's value[3], and extract if it reaches 0
 * ^ decrease component's value[4], and extract if it reaches 0
 * & decrease component's value[5], and extract if it reaches 0
 */
bool process_spell_components( CHAR_DATA *ch, int sn )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    char                   *comp = skill->components;
    char                   *check;
    char                    arg[MIL];
    bool                    consume,
                            fail,
                            found;
    int                     val,
                            value;
    OBJ_DATA               *obj;

    /*
     * if no components necessary, then everything is cool 
     */
    if ( !comp || comp[0] == '\0' )
        return TRUE;

    while ( comp[0] != '\0' ) {
        comp = one_argument( comp, arg );
        consume = TRUE;
        fail = found = FALSE;
        val = -1;
        switch ( arg[1] ) {
            default:
                check = arg + 1;
                break;
            case '!':
                check = arg + 2;
                fail = TRUE;
                break;
            case '+':
                check = arg + 2;
                consume = FALSE;
                break;
            case '@':
                check = arg + 2;
                val = 0;
                break;
            case '#':
                check = arg + 2;
                val = 1;
                break;
            case '$':
                check = arg + 2;
                val = 2;
                break;
            case '%':
                check = arg + 2;
                val = 3;
                break;
            case '^':
                check = arg + 2;
                val = 4;
                break;
            case '&':
                check = arg + 2;
                val = 5;
                break;
                /*
                 * reserve '*', '(' and ')' for v6, v7 and v8   
                 */
        }
        value = atoi( check );
        obj = NULL;
        switch ( UPPER( arg[0] ) ) {
            case 'T':
                for ( obj = ch->first_carrying; obj; obj = obj->next_content )
                    if ( obj->item_type == value ) {
                        if ( fail ) {
                            send_to_char( "Something disrupts the casting of this spell...\r\n",
                                          ch );
                            return FALSE;
                        }
                        found = TRUE;
                        break;
                    }
                break;
            case 'V':
                for ( obj = ch->first_carrying; obj; obj = obj->next_content )
                    if ( obj->pIndexData->vnum == value ) {
                        if ( fail ) {
                            send_to_char( "Something disrupts the casting of this spell...\r\n",
                                          ch );
                            return FALSE;
                        }
                        found = TRUE;
                        break;
                    }
                break;
            case 'K':
                for ( obj = ch->first_carrying; obj; obj = obj->next_content )
                    if ( nifty_is_name( check, obj->name ) ) {
                        if ( fail ) {
                            send_to_char( "Something disrupts the casting of this spell...\r\n",
                                          ch );
                            return FALSE;
                        }
                        found = TRUE;
                        break;
                    }
                break;
            case 'G':
                if ( GET_MONEY( ch, DEFAULT_CURR ) >= value ) {
                    if ( fail ) {
                        send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                        return FALSE;
                    }
                    else {
                        if ( consume ) {
                            set_char_color( AT_GOLD, ch );
                            send_to_char( "You feel a little lighter...\r\n", ch );
                            GET_MONEY( ch, DEFAULT_CURR ) -= value;
                        }
                        continue;
                    }
                }
                break;
            case 'H':
                if ( ch->hit >= value ) {
                    if ( fail ) {
                        send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                        return FALSE;
                    }
                    else {
                        if ( consume ) {
                            set_char_color( AT_BLOOD, ch );
                            send_to_char( "You feel a little weaker...\r\n", ch );
                            ch->hit -= value;
                            update_pos( ch );
                        }
                        continue;
                    }
                }
                break;
        }
        /*
         * having this component would make the spell fail... if we get
         * here, then the caster didn't have that component 
         */
        if ( fail )
            continue;
        if ( !found ) {
            send_to_char( "Something is missing...\r\n", ch );
            return FALSE;
        }
        if ( obj ) {
            if ( val >= 0 && val < 6 ) {
                separate_obj( obj );
                if ( obj->value[val] <= 0 ) {
                    act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
                    act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
                    extract_obj( obj );
                    return FALSE;
                }
                else if ( --obj->value[val] == 0 ) {
                    act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj,
                         NULL, TO_CHAR );
                    act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj,
                         NULL, TO_ROOM );
                    extract_obj( obj );
                }
                else
                    act( AT_MAGIC, "$p glows briefly and a whisp of smoke rises from it.", ch, obj,
                         NULL, TO_CHAR );
            }
            else if ( consume ) {
                separate_obj( obj );
                act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj,
                     NULL, TO_CHAR );
                act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj,
                     NULL, TO_ROOM );
                extract_obj( obj );
            }
            else {
                int                     count = obj->count;

                obj->count = 1;
                act( AT_MAGIC, "$p glows briefly.", ch, obj, NULL, TO_CHAR );
                obj->count = count;
            }
        }
    }
    return TRUE;
}

int                     pAbort;

/*
 * Locate targets.
 */
/* Turn off annoying message and just abort if needed */
bool                    silence_locate_targets;

bool check_casting_distance( CHAR_DATA *ch )
{
    if ( ch->fighting != NULL && ch->fighting->distance < 3 ) {
        send_to_char( "You're too close to cast that.\r\n", ch );
        return TRUE;
    }
    else
        return FALSE;
}

void                   *locate_targets( CHAR_DATA *ch, char *arg, int sn, CHAR_DATA **victim,
                                        OBJ_DATA **obj )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    void                   *vo = NULL;

    *victim = NULL;
    *obj = NULL;

    switch ( skill->target ) {
        default:
            bug( "Do_cast: bad target for sn %d.", sn );
            return &pAbort;

        case TAR_IGNORE:
            break;

        case TAR_CHAR_OFFENSIVE:
            {
                if ( arg[0] == '\0' ) {
                    if ( ( *victim = who_fighting( ch ) ) == NULL ) {
                        if ( !silence_locate_targets )
                            send_to_char( "Cast the spell on whom?\r\n", ch );
                        return &pAbort;
                    }
                }
                else {
                    if ( ( *victim = get_char_room( ch, arg ) ) == NULL ) {
                        if ( !silence_locate_targets )
                            send_to_char( "They aren't here.\r\n", ch );
                        return &pAbort;
                    }
                }
            }

            /*
             * Offensive spells will choose the ch up to 92% of the time
             * * if the nuisance flag is set -- Shaddai 
             */
            if ( !IS_NPC( ch ) && ch->pcdata->nuisance && ch->pcdata->nuisance->flags > 5
                 && number_percent(  ) <
                 ( ( ( ch->pcdata->nuisance->flags - 5 ) * 8 ) + ch->pcdata->nuisance->power * 6 ) )
                *victim = ch;

            if ( is_safe( ch, *victim, TRUE ) )
                return &pAbort;

            if ( ch == *victim ) {
                if ( SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) ) {
                    if ( !silence_locate_targets )
                        send_to_char( "You can't cast this on yourself!\r\n", ch );
                    return &pAbort;
                }
                if ( !silence_locate_targets )
                    send_to_char( "Cast this on yourself?  Okay...\r\n", ch );
                /*
                 * send_to_char("You can't do that to yourself.\r\n", ch);
                 * return &pAbort;
                 */
            }

            if ( !IS_NPC( ch ) ) {
                if ( !IS_NPC( *victim )
                     && ( !who_fighting( *victim ) || who_fighting( *victim ) != ch ) ) {
                    if ( get_timer( ch, TIMER_PKILLED ) > 0 ) {
                        if ( !silence_locate_targets )
                            send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                        return &pAbort;
                    }

                    if ( get_timer( *victim, TIMER_PKILLED ) > 0 ) {
                        if ( !silence_locate_targets )
                            send_to_char( "This player has been killed in the last 5 minutes.\r\n",
                                          ch );
                        return &pAbort;
                    }

                    if ( xIS_SET( ch->act, PLR_NICE ) && ch != *victim ) {
                        if ( !silence_locate_targets )
                            send_to_char( "You are too nice to attack another player.\r\n", ch );
                        return &pAbort;
                    }

                    if ( !IS_NPC( *victim ) && ch != *victim
                         && ( !IS_PKILL( ch ) || !IS_PKILL( *victim ) ) ) {
                        if ( !silence_locate_targets )
                            send_to_char( "You must MURDER a player.\r\n", ch );
                        return &pAbort;
                    }

                    /*
                     * if(*victim != ch)
                     * {
                     * if(!silence_locate_targets)
                     * send_to_char("You really shouldn't do this to another player...\r\n", ch);
                     * else if(who_fighting(*victim) != ch)
                     * {
                     * return &pAbort;
                     * }
                     * }
                     */
                }

                if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == *victim ) {
                    if ( !silence_locate_targets )
                        send_to_char( "You can't do that on your own follower.\r\n", ch );
                    return &pAbort;
                }
            }

            check_illegal_pk( ch, *victim );
            vo = ( void * ) *victim;
            break;

        case TAR_CHAR_DEFENSIVE:
            {
                if ( arg[0] == '\0' )
                    *victim = ch;
                else {
                    if ( ( *victim = get_char_room( ch, arg ) ) == NULL ) {
                        if ( !silence_locate_targets )
                            send_to_char( "They aren't here.\r\n", ch );
                        return &pAbort;
                    }
                }
            }

            /*
             * Nuisance flag will pick who you are fighting for defensive
             * * spells up to 36% of the time -- Shaddai
             */

            if ( !IS_NPC( ch ) && ch->fighting && ch->pcdata->nuisance
                 && ch->pcdata->nuisance->flags > 5
                 && number_percent(  ) <
                 ( ( ( ch->pcdata->nuisance->flags - 5 ) * 8 ) + 6 * ch->pcdata->nuisance->power ) )
                *victim = who_fighting( ch );

            if ( ch == *victim && SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) ) {
                if ( !silence_locate_targets )
                    send_to_char( "You can't cast this on yourself!\r\n", ch );
                return &pAbort;
            }

            vo = ( void * ) *victim;
            break;

        case TAR_CHAR_SELF:
            if ( arg[0] != '\0' && !nifty_is_name( arg, ch->name ) ) {
                if ( !silence_locate_targets )
                    send_to_char( "You cannot cast this spell on another.\r\n", ch );
                return &pAbort;
            }

            vo = ( void * ) ch;
            break;

        case TAR_OBJ_INV:
            {
                if ( arg[0] == '\0' ) {
                    if ( !silence_locate_targets )
                        send_to_char( "What should the spell be cast upon?\r\n", ch );
                    return &pAbort;
                }

                if ( ( *obj = get_obj_carry( ch, arg ) ) == NULL ) {
                    if ( !silence_locate_targets )
                        send_to_char( "You are not carrying that.\r\n", ch );
                    return &pAbort;
                }
            }

            vo = ( void * ) *obj;
            break;
    }

    return vo;
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char                   *target_name;
char                   *ranged_target_name = NULL;

/*
 * Cast a spell.  Multi-caster and component support by Thoric
 */
void do_cast( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    static char             staticbuf[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    void                   *vo = NULL;
    int                     mana;
    int                     blood;
    int                     sn;
    int                     xFactor;
    ch_ret                  retcode;
    bool                    dont_wait = FALSE;
    SKILLTYPE              *skill = NULL;
    struct timeval          time_used;

    retcode = rNONE;

    if ( check_casting_distance( ch ) == TRUE )
        return;

    switch ( ch->substate ) {
        default:
            /*
             * no ordering charmed mobs to cast spells 
             */

            if ( IS_NPC( ch )
                 && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) ) {
                send_to_char( "You can't seem to do that right now...\r\n", ch );
                return;
            }

            if ( ch->position == POS_MEDITATING ) {
                send_to_char( "You are concentrating too much for that!\r\n", ch );
                return;
            }
            if ( IS_AFFECTED( ch, AFF_SOUND_WAVES ) ) {
                send_to_char( "You can not concentrate enough for that!\r\n", ch );
                return;
            }
            if ( IS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC ) ) {
                set_char_color( AT_MAGIC, ch );
                send_to_char( "Something is this place is preventing your magic from working.\r\n",
                              ch );
                return;
            }

            target_name = one_argument( argument, arg1 );
            one_argument( target_name, arg2 );
            if ( ranged_target_name )
                STRFREE( ranged_target_name );
            ranged_target_name = STRALLOC( target_name );

            if ( arg1[0] == '\0' ) {
                send_to_char( "Cast which what where?\r\n", ch );
                return;
            }

            /*
             * Regular mortal spell casting 
             */
            if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
                if ( ( sn = find_spell( ch, arg1, TRUE ) ) < 0
                     || ( !IS_NPC( ch ) && !CAN_LEARN( ch, sn, TRUE ) ) ) {
                    send_to_char( "&BYou can't do that.\r\n", ch );
                    return;
                }
                if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                    send_to_char( "You can't do that right now...\r\n", ch );
                    return;
                }
            }
            else
                /*
                 * Godly "spell builder" spell casting with debugging messages
                 */
            {
                if ( ( sn = skill_lookup( arg1 ) ) < 0 ) {
                    send_to_char( "We didn't create that yet...\r\n", ch );
                    return;
                }
                if ( sn >= MAX_SKILL ) {
                    send_to_char( "Hmm... that might hurt.\r\n", ch );
                    return;
                }
                if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                    send_to_char( "Something is severely wrong with that one...\r\n", ch );
                    return;
                }
                if ( skill->type != SKILL_SPELL ) {
                    send_to_char( "That isn't a spell.\r\n", ch );
                    return;
                }
                if ( !skill->spell_fun ) {
                    send_to_char( "We didn't finish that one yet...\r\n", ch );
                    return;
                }
            }

            /*
             * Something else removed by Merc   -Thoric
             */
            /*
             * Band-aid alert!  !IS_NPC check -- Blod 
             */
            if ( ch->position < skill->minimum_position && !IS_NPC( ch ) ) {
                switch ( ch->position ) {
                    default:
                        send_to_char( "You can't concentrate enough.\r\n", ch );
                        break;
                    case POS_SITTING:
                        send_to_char( "You can't summon enough energy sitting down.\r\n", ch );
                        break;
                    case POS_RESTING:
                        send_to_char( "You're too relaxed to cast that spell.\r\n", ch );
                        break;
                    case POS_FIGHTING:
                        if ( skill->minimum_position <= POS_EVASIVE ) {
                            send_to_char( "This fighting style is too demanding for that!\r\n",
                                          ch );
                        }
                        else {
                            send_to_char( "No way!  You are still fighting!\r\n", ch );
                        }
                        break;
                    case POS_DEFENSIVE:
                        if ( skill->minimum_position <= POS_EVASIVE ) {
                            send_to_char( "This fighting style is too demanding for that!\r\n",
                                          ch );
                        }
                        else {
                            send_to_char( "No way!  You are still fighting!\r\n", ch );
                        }
                        break;
                    case POS_AGGRESSIVE:
                        if ( skill->minimum_position <= POS_EVASIVE ) {
                            send_to_char( "This fighting style is too demanding for that!\r\n",
                                          ch );
                        }
                        else {
                            send_to_char( "No way!  You are still fighting!\r\n", ch );
                        }
                        break;
                    case POS_BERSERK:
                        if ( skill->minimum_position <= POS_EVASIVE ) {
                            send_to_char( "This fighting style is too demanding for that!\r\n",
                                          ch );
                        }
                        else {
                            send_to_char( "No way!  You are still fighting!\r\n", ch );
                        }
                        break;
                    case POS_EVASIVE:
                        send_to_char( "No way!  You are still fighting!\r\n", ch );
                        break;
                    case POS_MEDITATING:
                        send_to_char( "You are concentrating too much to perform that.\r\n", ch );
                        break;
                    case POS_SLEEPING:
                        send_to_char( "You dream about great feats of magic.\r\n", ch );
                        break;
                }
                return;
            }

            if ( skill->spell_fun == spell_null ) {
                send_to_char( "That's not a spell!\r\n", ch );
                return;
            }

            if ( !skill->spell_fun ) {
                send_to_char( "You cannot cast that... yet.\r\n", ch );
                return;
            }

            /*
             * Volk - let's have checks for verbal and somatic spells! 
             */
            if ( !can_speak( ch ) && skill->verbal == TRUE ) {
                send_to_char( "That spell has a verbal requirement - but you can't speak!\r\n",
                              ch );
                return;
            }

            if ( !can_move( ch ) && skill->somatic == TRUE ) {
                send_to_char( "That spell requires hand gestures, and you're paralysed!\r\n", ch );
                return;
            }

            /*
             * Mystaric, 980908 - Added checks for spell sector type 
             */
            if ( !ch->in_room
                 || ( skill->spell_sector
                      && !IS_SET( skill->spell_sector, ( 1 << ch->in_room->sector_type ) ) ) ) {
                send_to_char( "You can not cast that here.\r\n", ch );
                return;
            }

/*    Volk: This is crashing the mud on occasion, why? 
        mana = IS_NPC(ch) ? 0 : UMAX(skill->min_mana, 100 / (2 + ch->level - get_maxskill(ch, sn)));
      Wonder if this will fix it.. But it might be the get_maxskill function.*/
/*
         if ( IS_NPC ( ch ) )
            mana = 0;
         else
         {
            int x = ( 2 + ch->level ) - get_maxskill ( ch, sn, TRUE );

            if ( x == 0 )
            {
               x = 1;
            }
            mana = UMAX ( skill->min_mana, ( 100 / x ) );
         }
*/
            if ( IS_NPC( ch ) ) {
                mana = 0;
            }
            else {
                mana = skill_table[sn]->min_mana;
            }

            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
                send_to_char
                    ( "You send a surge of mana into the spell invoking the higher magic.\r\n",
                      ch );
                mana = mana * 2;
            }

            /*
             * Locate targets.
             */
            vo = locate_targets( ch, arg2, sn, &victim, &obj );
            if ( vo == &pAbort )
                return;

            if ( SPELL_FLAG( skill_table[sn], SF_HEAL ) && !victim ) {
                victim = ch;
            }
            if ( SPELL_FLAG( skill_table[sn], SF_HEAL ) && victim->hit >= victim->max_hit ) {
                send_to_char( "This one is already at max health.\r\n", ch );
                return;
            }

            /*
             * Vampire spell casting     -Thoric
             */
            blood = UMAX( 1, ( mana + 4 ) / 8 );
            if ( IS_BLOODCLASS( ch ) ) {
                if ( ch->blood < blood ) {
                    send_to_char( "You don't have enough blood power.\r\n", ch );
                    return;
                }
            }
            else if ( !IS_NPC( ch ) && ch->mana < mana ) {
                send_to_char( "You don't have enough mana.\r\n", ch );
                return;
            }

            if ( skill->participants <= 1 )
                break;

            /*
             * multi-participant spells   -Thoric 
             */
            add_timer( ch, TIMER_DO_FUN, UMIN( skill->beats / 10, 3 ), do_cast, 1 );
            act( AT_MAGIC, "You begin to chant...", ch, NULL, NULL, TO_CHAR );
            act( AT_MAGIC, "$n begins to chant...", ch, NULL, NULL, TO_ROOM );
            snprintf( staticbuf, MIL, "%s %s", arg2, target_name );
            ch->alloc_ptr = str_dup( staticbuf );
            ch->tempnum = sn;
            return;
        case SUB_TIMER_DO_ABORT:
            DISPOSE( ch->alloc_ptr );
            if ( IS_VALID_SN( ( sn = ch->tempnum ) ) ) {
                if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                    send_to_char( "Something went wrong...\r\n", ch );
                    bug( "do_cast: SUB_TIMER_DO_ABORT: bad sn %d", sn );
                    return;
                }
/*
            mana =
               IS_NPC ( ch ) ? 0 : UMAX ( skill->min_mana,
                                          100 / ( 2 + ch->level -
                                                  get_maxskill ( ch, sn, TRUE ) ) );
*/
                if ( IS_NPC( ch ) ) {
                    mana = 0;
                }
                else {
                    mana = skill_table[sn]->min_mana;
                }

                blood = UMAX( 1, ( mana + 4 ) / 8 );
                if ( IS_BLOODCLASS( ch ) )
                    ch->blood = ch->blood - UMAX( 1, blood / 3 );
                else if ( ch->level < LEVEL_IMMORTAL ) /* so imms dont lose mana */
                    ch->mana -= mana / 3;
            }
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You stop chanting...\r\n", ch );
            /*
             * should add chance of backfire here 
             */
            return;
        case 1:
            sn = ch->tempnum;
            if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                send_to_char( "Something went wrong...\r\n", ch );
                bug( "do_cast: substate 1: bad sn %d", sn );
                return;
            }
            if ( !ch->alloc_ptr || !IS_VALID_SN( sn ) || skill->type != SKILL_SPELL ) {
                send_to_char( "Something cancels out the spell!\r\n", ch );
                bug( "do_cast: ch->alloc_ptr NULL or bad sn (%d)", sn );
                return;
            }
/*
         mana =
            IS_NPC ( ch ) ? 0 : UMAX ( skill->min_mana,
                                       100 / ( 2 + ch->level -
                                               get_maxskill ( ch, sn, TRUE ) ) ); */
            if ( IS_NPC( ch ) ) {
                mana = 0;
            }
            else {
                mana = skill_table[sn]->min_mana;
            }

            blood = UMAX( 1, ( mana + 4 ) / 8 );
            mudstrlcpy( staticbuf, ch->alloc_ptr, MIL );
            target_name = one_argument( staticbuf, arg2 );
            DISPOSE( ch->alloc_ptr );
            ch->substate = SUB_NONE;
            if ( skill->participants > 1 ) {
                int                     cnt = 1;
                CHAR_DATA              *tmp;
                TIMER                  *t;

                for ( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
                    if ( tmp != ch && ( t = get_timerptr( tmp, TIMER_DO_FUN ) ) != NULL
                         && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn
                         && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                        ++cnt;
                if ( cnt >= skill->participants ) {
                    for ( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
                        if ( tmp != ch && ( t = get_timerptr( tmp, TIMER_DO_FUN ) ) != NULL
                             && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn
                             && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) ) {
                            extract_timer( tmp, t );
                            act( AT_MAGIC,
                                 "Channeling your energy into $n, you help cast the spell!", ch,
                                 NULL, tmp, TO_VICT );
                            act( AT_MAGIC, "$N channels $S energy into you!", ch, NULL, tmp,
                                 TO_CHAR );
                            act( AT_MAGIC, "$N channels $S energy into $n!", ch, NULL, tmp,
                                 TO_NOTVICT );
                            learn_from_success( tmp, sn );
                            if ( IS_BLOODCLASS( ch ) )
                                ch->blood = ch->blood - blood;
                            else
                                tmp->mana -= mana;
                            tmp->substate = SUB_NONE;
                            tmp->tempnum = -1;
                            DISPOSE( tmp->alloc_ptr );
                        }
                    dont_wait = TRUE;
                    send_to_char
                        ( "You concentrate all the energy into a burst of mystical words!\r\n",
                          ch );
                    vo = locate_targets( ch, arg2, sn, &victim, &obj );
                    if ( vo == &pAbort )
                        return;
                }
                else {
                    set_char_color( AT_MAGIC, ch );
                    send_to_char( "There was not enough power for the spell to succeed...\r\n",
                                  ch );
                    if ( IS_BLOODCLASS( ch ) )
                        ch->blood = ch->blood - UMAX( 1, blood / 2 );
                    else if ( ch->level < LEVEL_IMMORTAL )  /* so imms dont lose mana */
                        ch->mana -= mana / 2;
                    learn_from_failure( ch, sn );
                    return;
                }
            }
    }

    /*
     * uttering those magic words unless casting "ventriloquate" 
     */
    if ( str_cmp( skill->name, "ventriloquate" ) )
        say_spell( ch, sn );

    if ( !dont_wait )
        WAIT_STATE( ch, skill->beats );

    /*
     * Getting ready to cast... check for spell components  -Thoric
     */
    if ( !process_spell_components( ch, sn ) ) {
        if ( IS_BLOODCLASS( ch ) )
            ch->blood = ch->blood - UMAX( 1, blood / 2 );
        else if ( ch->level < LEVEL_IMMORTAL )         /* so imms dont lose mana */
            ch->mana -= mana / 2;
        learn_from_failure( ch, sn );
        return;
    }

    if ( !IS_THIRDCLASS( ch ) && !IS_SECONDCLASS( ch )
         && ( ch->Class != CLASS_WARRIOR && ch->Class != CLASS_MONK
              && ch->Class != CLASS_SHADOWKNIGHT && ch->Class != CLASS_CRUSADER ) )
        skill->difficulty = 0;
    if ( !IS_THIRDCLASS( ch )
         && ( ch->Class != CLASS_WARRIOR && ch->Class != CLASS_MONK
              && ch->Class != CLASS_SHADOWKNIGHT && ch->Class != CLASS_CRUSADER
              && ch->secondclass != CLASS_WARRIOR && ch->secondclass != CLASS_MONK
              && ch->secondclass != CLASS_SHADOWKNIGHT && ch->secondclass != CLASS_CRUSADER ) )
        skill->difficulty = 1;
    if ( IS_THIRDCLASS( ch )
         && ( ch->Class != CLASS_WARRIOR && ch->Class != CLASS_MONK
              && ch->Class != CLASS_SHADOWKNIGHT && ch->Class != CLASS_CRUSADER )
         && ( ch->secondclass != CLASS_WARRIOR && ch->secondclass != CLASS_MONK
              && ch->secondclass != CLASS_SHADOWKNIGHT
              && ch->secondclass != CLASS_CRUSADER ) && ( ch->thirdclass != CLASS_WARRIOR
                                                          && ch->thirdclass != CLASS_MONK
                                                          && ch->thirdclass != CLASS_SHADOWKNIGHT
                                                          && ch->thirdclass != CLASS_CRUSADER ) )
        skill->difficulty = 2;

    if ( !IS_THIRDCLASS( ch ) && !IS_SECONDCLASS( ch )
         && ( ch->Class == CLASS_WARRIOR || ch->Class == CLASS_MONK
              || ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_CRUSADER ) )
        skill->difficulty = 3;
    if ( !IS_THIRDCLASS( ch )
         && ( ch->Class == CLASS_WARRIOR || ch->Class == CLASS_MONK
              || ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_CRUSADER
              || ch->secondclass == CLASS_WARRIOR || ch->secondclass == CLASS_MONK
              || ch->secondclass == CLASS_SHADOWKNIGHT || ch->secondclass == CLASS_CRUSADER ) )
        skill->difficulty = 4;
    if ( IS_THIRDCLASS( ch ) && ( ch->Class == CLASS_WARRIOR || ch->Class == CLASS_MONK ||
                                  ch->Class == CLASS_SHADOWKNIGHT
                                  || ch->Class == CLASS_CRUSADER
                                  || ch->secondclass == CLASS_WARRIOR
                                  || ch->secondclass == CLASS_MONK
                                  || ch->secondclass == CLASS_SHADOWKNIGHT
                                  || ch->secondclass == CLASS_CRUSADER
                                  || ch->thirdclass == CLASS_WARRIOR || ch->thirdclass == CLASS_MONK
                                  || ch->thirdclass == CLASS_SHADOWKNIGHT
                                  || ch->thirdclass == CLASS_CRUSADER ) )
        skill->difficulty = 5;

    if ( !IS_NPC( ch )
         && ( number_percent(  ) + ( skill->difficulty ) + number_range( 1, 6 ) ) >
         ch->pcdata->learned[sn] ) {
        /*
         * Some more interesting loss of concentration messages  -Thoric 
         */
        switch ( number_bits( 2 ) ) {
            case 0:                                   /* too busy */
                if ( ch->fighting )
                    send_to_char( "This round of battle is too hectic to concentrate properly.\r\n",
                                  ch );
                else
                    send_to_char( "You lost your concentration.\r\n", ch );
                break;
            case 1:                                   /* irritation */
                if ( number_bits( 2 ) == 0 ) {
                    switch ( number_bits( 2 ) ) {
                        case 0:
                            send_to_char
                                ( "A tickle in your nose prevents you from keeping your concentration.\r\n",
                                  ch );
                            break;
                        case 1:
                            send_to_char
                                ( "An itch on your leg keeps you from properly casting your spell.\r\n",
                                  ch );
                            break;
                        case 2:
                            send_to_char
                                ( "Something in your throat prevents you from uttering the proper phrase.\r\n",
                                  ch );
                            break;
                        case 3:
                            send_to_char
                                ( "A twitch in your eye disrupts your concentration for a moment.\r\n",
                                  ch );
                            break;
                    }
                }
                else
                    send_to_char( "Something distracts you, and you lose your concentration.\r\n",
                                  ch );
                break;
            case 2:                                   /* not enough time */
                if ( ch->fighting )
                    send_to_char
                        ( "There wasn't enough time this round to complete the casting.\r\n", ch );
                else
                    send_to_char( "You lost your concentration.\r\n", ch );
                break;
            case 3:
                send_to_char( "You get a mental block mid-way through the casting.\r\n", ch );
                break;
        }
        if ( IS_BLOODCLASS( ch ) )
            ch->blood = ch->blood - UMAX( 1, blood / 2 );
        else if ( ch->level < LEVEL_IMMORTAL )         /* so imms dont lose mana */
            ch->mana -= mana / 2;
        learn_from_failure( ch, sn );
        return;
    }
    else {
        if ( IS_BLOODCLASS( ch ) )
            ch->blood = ch->blood - blood;
        else
            ch->mana -= mana;

        // ENERGY CONTAINMENT SUPPORT COMPLETED - Taon
        if ( ch != victim && skill->target == TAR_CHAR_OFFENSIVE
             && IS_AFFECTED( victim, AFF_ENERGY_CONTAINMENT )
             && victim->position != POS_SLEEPING && victim->mana >= 25 ) {
            if ( !IS_NPC( victim ) ) {
                if ( victim->pcdata->learned[gsn_energy_containment] <= 40 )
                    xFactor = 9;
                else if ( victim->pcdata->learned[gsn_energy_containment] <= 60 )
                    xFactor = 8;
                else if ( victim->pcdata->learned[gsn_energy_containment] <= 90 )
                    xFactor = 7;
                else
                    xFactor = 6;
            }
            else
                xFactor = 9;

            if ( number_chance( 1, 10 ) >= xFactor ) {
                if ( IS_NPC( victim ) )
                    ch_printf( ch, "%s's energy containment absorbs your spell!\r\n",
                               victim->short_descr );
                else
                    ch_printf( ch, "%s's energy containment absorbs your spell!\r\n",
                               victim->name );

                if ( !IS_NPC( ch ) )
                    ch_printf( victim, "Your energy containment absorbs %s's spell.\r\n",
                               ch->name );
                else
                    ch_printf( victim, "Your energy containment absorbs %s's spell.\r\n",
                               ch->short_descr );

                if ( victim->mana <= 30 ) {
                    victim->mana = 0;
                    send_to_char( "Your energy containment falters as you run out of mana.\r\n",
                                  victim );
                    xREMOVE_BIT( victim->affected_by, AFF_ENERGY_CONTAINMENT );
                }
                else
                    victim->mana -= number_chance( 10, 30 );

                learn_from_success( victim, gsn_energy_containment );
                return;
            }
        }

        /*
         * check for immunity to magic if victim is known...
         * and it is a TAR_CHAR_DEFENSIVE/SELF spell
         * otherwise spells will have to check themselves
         */
        if ( ( ( skill->target == TAR_CHAR_DEFENSIVE || skill->target == TAR_CHAR_SELF ) && victim
               && IS_IMMUNE( victim, RIS_MAGIC ) ) ) {
            immune_casting( skill, ch, victim, NULL );
            retcode = rSPELL_FAILED;
        }
        else {
            start_timer( &time_used );
            retcode = ( *skill->spell_fun ) ( sn, ch->level, ch, vo );
            end_timer( &time_used );
            update_userec( &time_used, &skill->userec );
        }
    }

    if ( ch->in_room && IS_SET( ch->in_room->area->flags, AFLAG_SPELLLIMIT ) )
        ch->in_room->area->curr_spell_count++;

    if ( retcode == rCHAR_DIED || retcode == rERROR || char_died( ch ) )
        return;

    /*
     * learning 
     */
    if ( retcode != rSPELL_FAILED ) {
        /*
         * If spell uses 20 mana and ch level is 20, gain 2000 experience (20x20/2) 50
         * mana, level 100 - gain 5000 xp. 200 mana, level 50 - gain 10000 xp. 
         */
        int                     xpmage = mana * ( ch->level * 2 );

        if ( xpmage > 20000 )
            xpmage = 20000 + number_range( 1, 3000 );
        if ( !IS_NPC( ch ) && ch->Class == CLASS_MAGE && xpmage > 0 )
            gain_exp( ch, xpmage );
        learn_from_success( ch, sn );
    }
    else
        learn_from_failure( ch, sn );

    /*
     * favor adjustments
     */
    if ( victim && victim != ch && !IS_NPC( victim ) && skill->target == TAR_CHAR_DEFENSIVE )
        adjust_favor( ch, 7, 1 );

    if ( victim && victim != ch && !IS_NPC( ch ) && skill->target == TAR_CHAR_DEFENSIVE )
        adjust_favor( victim, 13, 1 );

    if ( victim && victim != ch && !IS_NPC( ch ) && skill->target == TAR_CHAR_OFFENSIVE )
        adjust_favor( ch, 4, 1 );

    /*
     * Fixed up a weird mess here, and added double safeguards  -Thoric
     */
    if ( skill->target == TAR_CHAR_OFFENSIVE && victim && !char_died( victim ) && victim != ch ) {
        CHAR_DATA              *vch,
                               *vch_next;

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( vch == victim ) {
                if ( vch->master != ch && !vch->fighting )
                    retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}

/*
 * Cast spells at targets using a magical object.
 */
ch_ret obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    void                   *vo;
    ch_ret                  retcode = rNONE;
    int                     levdiff = ch->level - level;
    SKILLTYPE              *skill = get_skilltype( sn );
    struct timeval          time_used;

    if ( sn == -1 || sn == 0 )
        return retcode;
    if ( !skill || !skill->spell_fun ) {
        bug( "Obj_cast_spell: bad sn %d.", sn );
        return rERROR;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "Nothing seems to happen...\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) && skill->target == TAR_CHAR_OFFENSIVE ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "Nothing seems to happen...\r\n", ch );
        return rNONE;
    }

    /*
     * Basically this was added to cut down on level 5 players using level
     * 40 scrolls in battle too often ;)  -Thoric
     */
    if ( ( skill->target == TAR_CHAR_OFFENSIVE || number_bits( 7 ) == 1 )   /* 1/128
                                                                             * chance *
                                                                             * if *
                                                                             * non-offensive 
                                                                             */
         &&!chance( ch, 95 + levdiff ) ) {
        switch ( number_bits( 2 ) ) {
            case 0:
                failed_casting( skill, ch, victim, NULL );
                break;
            case 1:
                act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
                if ( victim )
                    act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
                act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
                return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
            case 2:
                failed_casting( skill, ch, victim, NULL );
                break;
            case 3:
                act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
                if ( victim )
                    act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
                act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
                return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
        }
        return rNONE;
    }

    target_name = ( char * ) "";
    switch ( skill->target ) {
        default:
            bug( "Obj_cast_spell: bad target for sn %d.", sn );
            return rERROR;

        case TAR_IGNORE:
            vo = NULL;
            if ( victim )
                target_name = victim->name;
            else if ( obj )
                target_name = obj->name;
            break;

        case TAR_CHAR_OFFENSIVE:
            if ( victim != ch ) {
                if ( !victim )
                    victim = who_fighting( ch );
                if ( !victim || ( !IS_NPC( victim ) && !in_arena( victim ) ) ) {
                    send_to_char( "You can't do that.\r\n", ch );
                    return rNONE;
                }
            }
            if ( ch != victim && is_safe( ch, victim, TRUE ) )
                return rNONE;
            vo = ( void * ) victim;
            break;

        case TAR_CHAR_DEFENSIVE:
            if ( victim == NULL )
                victim = ch;
            vo = ( void * ) victim;
            if ( IS_IMMUNE( victim, RIS_MAGIC ) ) {
                immune_casting( skill, ch, victim, NULL );
                return rNONE;
            }
            break;

        case TAR_CHAR_SELF:
            vo = ( void * ) ch;
            if ( IS_IMMUNE( ch, RIS_MAGIC ) ) {
                immune_casting( skill, ch, victim, NULL );
                return rNONE;
            }
            break;

        case TAR_OBJ_INV:
            if ( obj == NULL ) {
                send_to_char( "You can't do that.\r\n", ch );
                return rNONE;
            }
            vo = ( void * ) obj;
            break;
    }

    start_timer( &time_used );
    retcode = ( *skill->spell_fun ) ( sn, level, ch, vo );
    end_timer( &time_used );
    update_userec( &time_used, &skill->userec );

    if ( retcode == rSPELL_FAILED )
        retcode = rNONE;

    if ( retcode == rCHAR_DIED || retcode == rERROR )
        return retcode;

    if ( char_died( ch ) )
        return rCHAR_DIED;

    if ( skill->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) ) {
        CHAR_DATA              *vch;
        CHAR_DATA              *vch_next;

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;
            if ( victim == vch && !vch->fighting && vch->master != ch ) {
                retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return retcode;
}

/*
 * Spell functions.
 */
ch_ret spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    dam = mediumhigh;
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    af.type = sn;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.level = ch->level;
    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        af.duration = ( 2 * level );
    }
    else {
        af.duration = ( 1 + level );
    }
    af.bitvector = meb( AFF_BLINDNESS );
    affect_to_char( victim, &af );
    set_char_color( AT_MAGIC, victim );
    act( AT_MAGIC, "$n weaves a spell of blindness about you.", ch, NULL, ch, TO_VICT );
    send_to_char( "You are blinded!\r\n", victim );
    if ( ch != victim ) {
        act( AT_MAGIC, "You weave a spell of blindness around $N.", ch, NULL, victim, TO_CHAR );
        act( AT_MAGIC, "$n weaves a spell of blindness about $N.", ch, NULL, victim, TO_NOTVICT );
    }
    return rNONE;
}

ch_ret spell_burning_hands( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 0, 0, 14, 17, 20, 23, 26, 29,
        29, 29, 30, 30, 31, 31, 32, 32, 33, 33,
        34, 34, 35, 35, 36, 36, 37, 37, 38, 38,
        39, 39, 40, 40, 41, 41, 42, 42, 43, 43,
        44, 44, 45, 45, 46, 46, 47, 47, 48, 48,
        49, 49, 50, 50, 51, 51, 52, 52, 53, 53,
        54, 54, 55, 55, 56, 56, 57, 57, 58, 58
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );

    // A little more flaming shield support. -Taon
    if ( IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The fireball you cast cannot penetrate $N's flaming shield.", ch, NULL,
             victim, TO_CHAR );
        act( AT_RED, "$n's fireball couldn't penetrate your flaming shield.", ch, NULL, victim,
             TO_VICT );
        dam = 0;
    }
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The fireball you cast only partially penetrates $N's fireshield.", ch, NULL,
             victim, TO_CHAR );
        act( AT_RED, "$n's fireball only partially penetrates your fireshield.", ch, NULL, victim,
             TO_VICT );
        dam = number_range( dam_each[level] / 2, dam_each[level] * 3 );
    }
    else
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );

    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    int                     dam;
    bool                    ch_died;
    ch_ret                  retcode = rNONE;

    if ( !IS_OUTSIDE( ch ) ) {
        send_to_char( "You must be out of doors.\r\n", ch );
        return rSPELL_FAILED;
    }

    struct WeatherCell     *cell = getWeatherCell( ch->in_room->area );

    if ( getPrecip( cell ) < 40 && getEnergy( cell ) < 30 ) {
        send_to_char( "You need bad weather.\r\n", ch );
        return rSPELL_FAILED;
    }

    set_char_color( AT_MAGIC, ch );
    send_to_char( "You call upon lightning to strike your foes!\r\n", ch );
    act( AT_MAGIC, "$n calls upon lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM );

    ch_died = FALSE;
    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        if ( !vch->in_room )
            continue;

        if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
            continue;

        if ( IS_AFFECTED( vch, AFF_SHOCKSHIELD ) ) {
            act( AT_YELLOW, "The lightning you called only partially penetrates $N's shockshield.",
                 ch, NULL, vch, TO_CHAR );
            act( AT_YELLOW, "$n's lightning only partially penetrates your shockshield.", ch, NULL,
                 vch, TO_VICT );
            dam = dice( level / 3, 6 );
        }
        else {
            dam = dice( level / 2, 8 );
        }

        if ( vch->in_room == ch->in_room ) {
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;

            if ( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) )
                retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
            if ( retcode == rCHAR_DIED || char_died( ch ) )
                ch_died = TRUE;
            continue;
        }

        if ( !ch_died && vch->in_room->area == ch->in_room->area && IS_OUTSIDE( vch )
             && IS_AWAKE( vch ) ) {
            if ( number_bits( 3 ) == 0 )
                send_to_char_color( "&BLightning flashes in the sky.\r\n", vch );
        }
    }

    if ( ch_died )
        return rCHAR_DIED;
    else
        return rNONE;
}

ch_ret spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
    return damage( ch, ( CHAR_DATA * ) vo, dice( 1, 8 ) + level / 3, sn );
}

ch_ret spell_cause_critical( int sn, int level, CHAR_DATA *ch, void *vo )
{
    return damage( ch, ( CHAR_DATA * ) vo, dice( 3, 8 ) + level - 6, sn );
}

ch_ret spell_cause_serious( int sn, int level, CHAR_DATA *ch, void *vo )
{
    return damage( ch, ( CHAR_DATA * ) vo, dice( 2, 8 ) + level / 2, sn );
}

ch_ret spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    if ( is_affected( victim, sn ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    af.type = sn;
    af.duration = 10 * level;
    af.location = APPLY_SEX;
    af.level = ch->level;
    do {
        af.modifier = number_range( 0, 2 ) - victim->sex;
    }
    while ( af.modifier == 0 );
    xCLEAR_BITS( af.bitvector );
    affect_to_char( victim, &af );
    set_char_color( AT_MAGIC, victim );
    send_to_char( "You feel different.\r\n", victim );
    if ( ch != victim )
        send_to_char( "You have.\r\n", ch );
    successful_casting( skill, ch, victim, NULL );
    return rNONE;
}

bool can_charm( CHAR_DATA *ch )
{
    if ( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
        return TRUE;
    if ( ( ( get_curr_cha( ch ) / 3 ) + 1 ) > ch->pcdata->charmies )
        return TRUE;
    return FALSE;
}

ch_ret spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    int                     chance;
    char                    buf[MSL];
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( victim == ch ) {
        send_to_char( "You like yourself even better!\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( IS_SET( victim->immune, RIS_MAGIC ) || IS_SET( victim->immune, RIS_CHARM ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( victim ) && !IS_NPC( ch ) ) {
        send_to_char( "I don't think so.\r\n", ch );
        send_to_char( "You feel charmed. Not.n\r", victim );
        return rSPELL_FAILED;
    }

    chance = ris_save( victim, level, RIS_CHARM );

    if ( IS_AFFECTED( victim, AFF_CHARM ) || chance == 1000 || IS_AFFECTED( ch, AFF_CHARM )
         || level < victim->level || circle_follow( victim, ch ) || !can_charm( ch )
         || saves_spell_staff( chance, victim ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    af.type = sn;
    af.duration = ( int ) ( ( number_fuzzy( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV );
    af.location = 0;
    af.modifier = 0;
    af.level = ch->level;
    af.bitvector = meb( AFF_CHARM );
    affect_to_char( victim, &af );
    successful_casting( skill, ch, victim, NULL );

    snprintf( buf, MSL, "%s has charmed %s.", ch->name, victim->name );
    log_string_plus( buf, LOG_NORMAL, ch->level );
    if ( !IS_NPC( ch ) )
        ch->pcdata->charmies++;
    if ( IS_NPC( victim ) ) {
        start_hating( victim, ch );
        start_hunting( victim, ch );
    }
    return rNONE;
}

ch_ret spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 6, 7, 8, 9, 12, 13, 13, 13,
        14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
        20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 25, 26, 26, 26, 27,
        27, 28, 28, 29, 29, 30, 30, 31, 31, 32,
        32, 33, 34, 34, 35, 35, 36, 37, 37, 38
    };
    AFFECT_DATA             af;
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) ) {
        act( AT_LBLUE, "The chilling touch you cast only partially penetrates $N's iceshield.", ch,
             NULL, victim, TO_CHAR );
        act( AT_LBLUE, "$n's chilling touch only partially penetrates your iceshield.", ch, NULL,
             victim, TO_VICT );
        dam = number_range( dam_each[level] / 2, dam_each[level] );
    }
    else {
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    }
    if ( !saves_spell_staff( level, victim ) ) {
        af.type = sn;
        af.duration = 14;
        af.location = APPLY_STR;
        af.modifier = -1;
        af.level = ch->level;
        xCLEAR_BITS( af.bitvector );
        affect_join( victim, &af );
    }
    else {
        dam /= 2;
    }

    return damage( ch, victim, dam, sn );
}

ch_ret spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        30, 35, 40, 45, 50, 55, 55, 55, 56, 57,
        58, 58, 59, 60, 61, 61, 62, 63, 64, 64,
        65, 66, 67, 67, 68, 69, 70, 70, 71, 72,
        73, 73, 74, 75, 76, 76, 77, 78, 79, 79,
        80, 80, 81, 82, 82, 83, 83, 84, 85, 85,
        86, 86, 87, 88, 88, 89, 89, 90, 91, 91
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;

    return damage( ch, victim, dam, sn );
}

ch_ret spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    int                     change;
    struct WeatherCell     *cell = getWeatherCell( ch->in_room->area );

    change = URANGE( 5, number_range( 5, 15 ) + ( ch->level / 10 ), 15 );

    if ( !str_cmp( target_name, "warmer" ) )
        IncreaseTemp( cell, change );
    else if ( !str_cmp( target_name, "colder" ) )
        DecreaseTemp( cell, change );
    else if ( !str_cmp( target_name, "wetter" ) )
        IncreasePrecip( cell, change );
    else if ( !str_cmp( target_name, "drier" ) )
        DecreasePrecip( cell, change );
    else if ( !str_cmp( target_name, "stormier" ) )
        IncreaseEnergy( cell, change );
    else if ( !str_cmp( target_name, "calmer" ) )
        DecreaseEnergy( cell, change );
    else {
        send_to_char( "Do you want it to get warmer, colder, wetter, "
                      "drier, stormier, or calmer?\r\n", ch );
        return rSPELL_FAILED;
    }
    successful_casting( skill, ch, NULL, NULL );
    return rNONE;
}

ch_ret spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *food;
    int                     x = number_range( OBJ_VNUM_FOOD1, ( OBJ_VNUM_FOOD2 - 1 ) );
    int                     y = number_range( 1, 100 );

    if ( ( ch->carry_number + 1 ) > can_carry_n( ch ) ) {
        send_to_char( "You can't carry anything else.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( y > 1 )
        food = create_object( get_obj_index( x ), 0 );
    else
        food = create_object( get_obj_index( OBJ_VNUM_FOOD2 ), 0 );

    food->value[0] = 5 + level;
    act( AT_MAGIC, "$p appears in $n's hands.", ch, food, NULL, TO_ROOM );
    act( AT_MAGIC, "$p appears in your hands.", ch, food, NULL, TO_CHAR );
    obj_to_char( food, ch );
    return rNONE;
}

ch_ret spell_summon_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *light = create_object( get_obj_index( OBJ_VNUM_SUMMONLIGHT ), 0 );

    light->timer = ( ch->level * 2 );
    light->level = ch->level;
    light->owner = ch->name;

    act( AT_MAGIC, "$p appears before $n!", ch, light, NULL, TO_ROOM );
    act( AT_MAGIC, "$p appears before you.", ch, light, NULL, TO_CHAR );
    obj_to_room( light, ch->in_room );
    return rNONE;
}

ch_ret spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    struct WeatherCell     *cell = getWeatherCell( ch->in_room->area );
    int                     water;

    if ( obj->item_type != ITEM_DRINK_CON ) {
        send_to_char( "It is unable to hold water.\r\n", ch );
        return rSPELL_FAILED;
    }
    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 ) {
        send_to_char( "It contains some other liquid.\r\n", ch );
        return rSPELL_FAILED;
    }
    water = UMIN( level * ( getPrecip( cell ) >= 0 ? 4 : 2 ), obj->value[0] - obj->value[1] );
    if ( water > 0 ) {
        separate_obj( obj );
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if ( !is_name( "water", obj->name ) ) {
            char                    buf[MSL];

            snprintf( buf, MSL, "%s water", obj->name );
            STRFREE( obj->name );
            obj->name = STRALLOC( buf );
        }
        act( AT_MAGIC, "$p is filled.", ch, obj, NULL, TO_CHAR );
    }
    return rNONE;
}

ch_ret spell_cure_blindness( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    set_char_color( AT_MAGIC, ch );
    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( !is_affected( victim, gsn_blindness ) ) {
        if ( ch != victim )
            send_to_char( "You work your cure, but it has no apparent effect.\r\n", ch );
        else
            send_to_char( "You don't seem to be blind.\r\n", ch );
        return rSPELL_FAILED;
    }
    affect_strip( victim, gsn_blindness );
    set_char_color( AT_MAGIC, victim );
    send_to_char( "Your vision returns!\r\n", victim );
    if ( ch != victim )
        send_to_char( "You work your cure, restoring vision.\r\n", ch );
    return rNONE;
}

ch_ret spell_cure_affliction( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    set_char_color( AT_MAGIC, ch );
    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    affect_strip( victim, gsn_maim );
    affect_strip( victim, gsn_brittle_bone );
    affect_strip( victim, gsn_festering_wound );
    affect_strip( victim, gsn_poison );
    xREMOVE_BIT( victim->affected_by, AFF_BRITTLE_BONES );
    xREMOVE_BIT( victim->affected_by, AFF_FUNGAL_TOXIN );
    xREMOVE_BIT( victim->affected_by, AFF_MAIM );
    set_char_color( AT_MAGIC, victim );
    send_to_char( "Any minor afflictions you may have had is cured!\r\n", victim );
    if ( ch != victim )
        send_to_char( "You work your cure, removing any minor afflictions.\r\n", ch );
    return rNONE;
}

ch_ret spell_sacral_divinity( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ch->alignment < 350 ) {
        act( AT_MAGIC, "Your prayer goes unanswered.", ch, NULL, NULL, TO_CHAR );
        return rSPELL_FAILED;
    }
    if ( IS_SET( ch->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) )
        return rSPELL_FAILED;
    af.type = sn;
    af.duration = level * 3;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;
    af.bitvector = meb( AFF_SANCTUARY );
    affect_to_char( ch, &af );
    act( AT_MAGIC, "A shroud of glittering light slowly wraps itself about $n.", ch, NULL, NULL,
         TO_ROOM );
    act( AT_MAGIC, "A shroud of glittering light slowly wraps itself around you.", ch, NULL, NULL,
         TO_CHAR );
    return rNONE;
}

ch_ret spell_expurgation( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    if ( !is_affected( victim, gsn_poison ) )
        return rSPELL_FAILED;
    affect_strip( victim, gsn_poison );
    act( AT_MAGIC, "You speak an ancient prayer, begging your god for purification.", ch, NULL,
         NULL, TO_CHAR );
    act( AT_MAGIC, "$n speaks an ancient prayer begging $s god for purification.", ch, NULL, NULL,
         TO_ROOM );
    return rNONE;
}

ch_ret spell_bethsaidean_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    if ( !is_affected( victim, gsn_blindness ) )
        return rSPELL_FAILED;
    affect_strip( victim, gsn_blindness );
    set_char_color( AT_MAGIC, victim );
    send_to_char( "Your sight is restored!\r\n", victim );
    if ( ch != victim ) {
        act( AT_MAGIC, "$n lays $s hands over your eyes and concentrates...", ch, NULL, victim,
             TO_VICT );
        act( AT_MAGIC, "$n lays $s hands over $N's eyes and concentrates...", ch, NULL, victim,
             TO_NOTVICT );
        act( AT_MAGIC, "Laying your hands on $N's eyes, you pray to lift $S blindness.", ch, NULL,
             victim, TO_CHAR );
    }
    return rNONE;
}

ch_ret spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( is_affected( victim, gsn_poison ) ) {
        affect_strip( victim, gsn_poison );
        set_char_color( AT_MAGIC, victim );
        send_to_char( "A warm feeling runs through your body.\r\n", victim );
        victim->mental_state = URANGE( -100, victim->mental_state, -10 );
        if ( ch != victim ) {
            act( AT_MAGIC, "A flush of health washes over $N.", ch, NULL, victim, TO_NOTVICT );
            act( AT_MAGIC, "You lift the poison from $N's body.", ch, NULL, victim, TO_CHAR );
        }
        return rNONE;
    }
    else {
        set_char_color( AT_MAGIC, ch );
        if ( ch != victim )
            send_to_char( "You work your cure, but it has no apparent effect.\r\n", ch );
        else
            send_to_char( "You don't seem to be poisoned.\r\n", ch );
        return rSPELL_FAILED;
    }
}

ch_ret spell_curse( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_AFFECTED( victim, AFF_CURSE ) || saves_spell_staff( level, victim ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    af.type = sn;
    af.duration = ( 2 * level );
    af.location = APPLY_HITROLL;
    af.level = ch->level;
    af.modifier = -1;
    af.bitvector = meb( AFF_CURSE );
    affect_to_char( victim, &af );

    af.location = APPLY_SAVING_SPELL;
    af.modifier = 1;
    af.level = ch->level;
    affect_to_char( victim, &af );

    set_char_color( AT_MAGIC, victim );
    send_to_char( "You feel unclean.\r\n", victim );
    if ( ch != victim ) {
        act( AT_MAGIC, "You utter a curse upon $N.", ch, NULL, victim, TO_CHAR );
        act( AT_MAGIC, "$n utters a curse upon $N.", ch, NULL, victim, TO_NOTVICT );
    }
    return rNONE;
}

ch_ret spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;

    set_char_color( AT_MAGIC, ch );
    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD
         || obj->item_type == ITEM_COOK ) {
        if ( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
            send_to_char( "It looks undercooked.\r\n", ch );
        else if ( obj->value[3] != 0 )
            send_to_char( "You smell poisonous fumes.\r\n", ch );
        else
            send_to_char( "It looks very delicious.\r\n", ch );
    }
    else {
        send_to_char( "It doesn't look poisoned.\r\n", ch );
    }

    return rNONE;
}

ch_ret spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( !IS_NPC( ch ) && IS_EVIL( ch ) )
        victim = ch;

    if ( IS_GOOD( victim ) ) {
        act( AT_MAGIC, "Mnera protects $N.", ch, NULL, victim, TO_ROOM );
        return rSPELL_FAILED;
    }

    if ( IS_NEUTRAL( victim ) ) {
        act( AT_MAGIC, "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return rSPELL_FAILED;
    }

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    dam = dice( level, 4 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

/*
 * New version of dispel magic fixes alot of bugs, and allows players
 * to not lose the affects if they have the spell and the affect.
 * Also prints a message to the victim, and does various other things :)
 * Shaddai
 */

ch_ret spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     cnt = 0,
        affect_num,
        affected_by = 0;
    int                     chance;
    SKILLTYPE              *skill = get_skilltype( sn );
    AFFECT_DATA            *paf;
    bool                    found = FALSE;
    bool                    is_mage = FALSE;

    set_char_color( AT_MAGIC, ch );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_NPC( ch ) || ch->Class == CLASS_MAGE || ch->secondclass == CLASS_MAGE
         || ch->thirdclass == CLASS_MAGE )
        is_mage = TRUE;

    if ( !IS_NPC( ch ) )
        chance = LEARNED( ch, skill_lookup( "dispel magic" ) );
    else
        chance = number_range( 75, 90 );

    if ( is_mage )
        chance += 10;
    else
        chance += 5;

    chance -= ( get_curr_int( ch ) - ( get_curr_int( victim ) + number_range( 0, 5 ) ) );
    chance -= ( level - ( victim->level + number_range( 0, 5 ) ) );

    /*
     * Bug Fix to prevent possesed mobs from being dispelled -Shaddai 
     */
    if ( IS_NPC( victim ) && IS_AFFECTED( victim, AFF_POSSESS ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rVICT_IMMUNE;
    }
// stop players being changed to human priests - Vladaar
    if ( IS_AFFECTED( victim, AFF_DRAGONLORD ) ) {
        interpret( victim, ( char * ) "human" );
    }

    if ( ch == victim ) {
        if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
            affect_strip( ch, gsn_human_form );
            xREMOVE_BIT( ch->affected_by, AFF_DRAGONLORD );
        }

        if ( ch->first_affect ) {
            while ( ch->first_affect )
                affect_remove( ch, ch->first_affect );
        }

        send_to_char( "You pass your hands around your body...\r\n", ch );
        if ( !IS_NPC( ch ) )
            update_aris( victim );
        return rNONE;
    }

    if ( !is_mage && !IS_AFFECTED( ch, AFF_DETECT_MAGIC ) ) {
        send_to_char( "You don't sense a magical aura to dispel.\r\n", ch );
        return rERROR;
    }

    /*
     * Grab affected_by from mobs first 
     */
    if ( IS_NPC( victim ) && !xIS_EMPTY( victim->affected_by ) ) {
        found = FALSE;

        for ( ;; ) {
            affected_by = number_range( 0, MAX_AFFECTED_BY - 1 );
            if ( xIS_SET( victim->affected_by, affected_by ) ) {
                found = TRUE;
                break;
            }
            if ( cnt++ > 130 )
                break;
        }
        if ( found ) {                                 /* Ok lets see if it is a spell */
            for ( paf = victim->first_affect; paf; paf = paf->next ) {
                if ( xIS_SET( paf->bitvector, affected_by )
                     || ( paf->location == APPLY_EXT_AFFECT && paf->modifier == affected_by )
                     || ( affected_by < 32 && paf->location == APPLY_AFFECT
                          && IS_SET( paf->modifier, 1 << affected_by ) ) )
                    break;
            }
            if ( paf ) {
                if ( number_percent(  ) > chance || saves_spell_staff( level, victim ) ) {
                    if ( !dispel_casting( paf, ch, victim, -1, FALSE ) )
                        failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
                if ( SPELL_FLAG( get_skilltype( paf->type ), SF_NODISPEL ) ) {
                    if ( !dispel_casting( paf, ch, victim, -1, FALSE ) )
                        failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
                if ( !dispel_casting( paf, ch, victim, -1, TRUE ) )
                    successful_casting( skill, ch, victim, NULL );
                if ( paf->type == gsn_human_form ) {   /* Need to fully strip this */
                    affect_strip( victim, gsn_human_form );
                    xREMOVE_BIT( victim->affected_by, AFF_DRAGONLORD );
                }
                else
                    affect_remove( victim, paf );
            }
            else {
                if ( number_percent(  ) > chance || saves_spell_staff( level, victim ) ) {
                    if ( !dispel_casting( NULL, ch, victim, -1, FALSE ) )
                        failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
                if ( !dispel_casting( NULL, ch, victim, affected_by, TRUE ) )
                    successful_casting( skill, ch, victim, NULL );

                if ( affected_by == AFF_DRAGONLORD )   /* Need to strip all of it */
                    affect_strip( victim, gsn_human_form );

                xREMOVE_BIT( victim->affected_by, affected_by );
            }

            return rNONE;
        }
    }

    /*
     * Ok mob has no affected_by's or we didn't catch them lets go to
     * * first_affect. SHADDAI
     */

    if ( !victim->first_affect ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    cnt = 0;

    /*
     * Need to randomize the affects, yes you have to loop on average 1.5 times
     * but dispel magic only takes at worst case 256 uSecs so who cares :)
     * Shaddai
     */

    for ( paf = victim->first_affect; paf; paf = paf->next )
        cnt++;

    paf = victim->first_affect;

    for ( affect_num = number_range( 0, ( cnt - 1 ) ); affect_num > 0; affect_num-- )
        paf = paf->next;

    if ( !IS_NPC( ch ) ) {
        if ( level < victim->level - number_range( 0, 5 ) || saves_spell_staff( level, victim ) ) {
            if ( !dispel_casting( paf, ch, victim, -1, FALSE ) )
                failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }
    }

    /*
     * Need to make sure we have an affect and it isn't no dispel 
     */
    if ( !IS_NPC( ch ) ) {
        if ( !paf || SPELL_FLAG( get_skilltype( paf->type ), SF_NODISPEL ) ) {
            if ( !dispel_casting( paf, ch, victim, -1, FALSE ) )
                failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }
    }

    if ( !dispel_casting( paf, ch, victim, -1, TRUE ) && !IS_NPC( ch ) )
        successful_casting( skill, ch, victim, NULL );

    affect_remove( victim, paf );

    /*
     * Have to reset victim affects 
     */

    if ( !IS_NPC( victim ) )
        update_aris( victim );

    return rNONE;
}

ch_ret spell_polymorph( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA             *morph;
    SKILLTYPE              *skill = get_skilltype( sn );

    morph = find_morph( ch, target_name, TRUE );
    if ( !morph ) {
        send_to_char( "You can't morph into anything like that!\r\n", ch );
        return rSPELL_FAILED;
    }
    if ( !do_morph_char( ch, morph ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    return rNONE;
}

ch_ret spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    bool                    ch_died;
    ch_ret                  retcode;
    SKILLTYPE              *skill = get_skilltype( sn );

    ch_died = FALSE;
    retcode = rNONE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }

    act( AT_MAGIC, "The earth trembles beneath your feet!", ch, NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        if ( !vch->in_room )
            continue;
        if ( vch->in_room == ch->in_room ) {
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            if ( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
                 && !IS_AFFECTED( vch, AFF_FLYING ) && !IS_AFFECTED( vch, AFF_FLOATING ) )
                retcode = damage( ch, vch, level + number_range( 5, 20 ), sn );
            if ( retcode == rCHAR_DIED || char_died( ch ) ) {
                ch_died = TRUE;
                continue;
            }
            if ( char_died( vch ) )
                continue;
        }

        if ( !ch_died && vch->in_room->area == ch->in_room->area ) {
            if ( number_bits( 3 ) == 0 )
                send_to_char_color( "&BThe earth trembles and shivers.\r\n", vch );
        }
    }

    if ( ch_died )
        return rCHAR_DIED;
    else
        return rNONE;
}

ch_ret spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA            *paf;

    if ( obj->item_type != ITEM_WEAPON || IS_OBJ_STAT( obj, ITEM_MAGIC ) || obj->first_affect
         || obj->level > ch->level ) {
        act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL,
             TO_CHAR );
        act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL,
             TO_NOTVICT );
        return rSPELL_FAILED;
    }

    /*
     * Bug fix here. -- Alty 
     */
    separate_obj( obj );
    CREATE( paf, AFFECT_DATA, 1 );
    paf->type = -1;
    paf->duration = -1;
    paf->location = APPLY_HITROLL;
    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        paf->modifier = level / 10;
    }
    else {
        paf->modifier = level / 15;
    }
    xCLEAR_BITS( paf->bitvector );
    LINK( paf, obj->first_affect, obj->last_affect, next, prev );

    CREATE( paf, AFFECT_DATA, 1 );

    paf->type = -1;
    paf->duration = -1;
    paf->location = APPLY_DAMROLL;
    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        paf->modifier = level / 10;
    }
    else {
        paf->modifier = level / 15;
    }

    xCLEAR_BITS( paf->bitvector );
    LINK( paf, obj->first_affect, obj->last_affect, next, prev );

    if ( IS_GOOD( ch ) ) {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
        act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_ROOM );
        act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_CHAR );
    }
    else if ( IS_EVIL( ch ) ) {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
        act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_CHAR );
        act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_ROOM );
    }
    else {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
        xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
        act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_ROOM );
        act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_CHAR );
    }
    return rNONE;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
ch_ret spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;
    int                     chance;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    // Modified formula.. -Taon
    chance =
        ris_save( victim, victim->level,
                  RIS_DRAIN ) + ( ch->level / 2 ) + ( get_curr_wis( ch ) * 4 );
    if ( chance == 1000 || saves_spell_staff( chance, victim ) ) {
        failed_casting( skill, ch, victim, NULL );     /* SB */
        return rSPELL_FAILED;
    }

    if ( number_percent(  ) < 25 )
        ch->alignment = UMAX( -1000, ( ch->alignment - 3 ) );

    if ( victim->level <= 2 )
        dam = ch->hit + 1;
    else {
        gain_exp( victim, 0 - number_range( level / 2, 3 * level / 2 ) );
        victim->mana -= 5;
        victim->move -= 5;
        dam = dice( 1, level );
        ch->hit += dam;
    }

    if ( ch->hit > ch->max_hit )
        ch->hit = ch->max_hit;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 30, 35, 40, 45, 50, 55,
        60, 65, 70, 75, 80, 82, 84, 86, 88, 90,
        92, 94, 96, 98, 100, 102, 104, 106, 108, 110,
        112, 114, 116, 118, 120, 122, 124, 126, 128, 130,
        132, 134, 136, 138, 140, 142, 144, 146, 148, 150,
        152, 154, 156, 158, 160, 162, 164, 166, 168, 170
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );

    // A little more flaming shield support. -Taon
    if ( IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The fireball you cast cannot penetrate $N's flaming shield.", ch, NULL,
             victim, TO_CHAR );
        act( AT_RED, "$n's fireball couldn't penetrate your flaming shield.", ch, NULL, victim,
             TO_VICT );
        dam = 0;
    }
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The fireball you cast only partially penetrates $N's fireshield.", ch, NULL,
             victim, TO_CHAR );
        act( AT_RED, "$n's fireball only partially penetrates your fireshield.", ch, NULL, victim,
             TO_VICT );
        dam = number_range( dam_each[level] / 2, dam_each[level] );
    }
    else {
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    }
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_paralyze( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;
    AFFECT_DATA             af;
    short                   chance;

    chance = number_range( 1, 5 );

    if ( IS_RESIS( victim, RIS_PARALYSIS ) )
        chance -= 1;

    if ( chance < 3 && !IS_NPC( victim ) ) {
        return rSPELL_FAILED;
    }

    if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
        if ( IS_NPC( victim ) )
            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
        else
            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 3 ) );
        af.type = gsn_paralyze;
        af.location = APPLY_AC;
        af.modifier = 20;
        af.duration = number_range( 1, 2 );
        af.level = ch->level;
        af.bitvector = meb( AFF_PARALYSIS );
        affect_to_char( victim, &af );
        update_pos( victim );
    }
    dam = number_chance( 1, 6 );

    return damage( ch, victim, dam, sn );
}

ch_ret spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    if ( IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The flamestrike you cast only partially penetrates $N's flaming shield.", ch,
             NULL, victim, TO_CHAR );
        act( AT_RED, "$n's flamestrike only partially penetrates your flaming shield.", ch, NULL,
             victim, TO_VICT );
        dam = dice( 1, 2 );
    }
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) ) {
        act( AT_RED, "The flamestrike you cast only partially penetrates $N's fireshield.", ch,
             NULL, victim, TO_CHAR );
        act( AT_RED, "$n's flamestrike only partially penetrates your fireshield.", ch, NULL,
             victim, TO_VICT );
        dam = dice( 3, 4 );
    }
    else {
        if ( ch->level >= 25 )
            dam = ( dice( 6, 8 ) + ( ch->level / 5 ) );
        else
            dam = dice( 6, 8 );
    }
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    af.type = sn;
    af.duration = ( int ) ( level * DUR_CONV );
    af.location = APPLY_AC;
    af.modifier = 2 * level;
    af.level = ch->level;
    af.bitvector = meb( AFF_FAERIE_FIRE );
    affect_to_char( victim, &af );
    act( AT_PINK, "You are surrounded by a pink outline.", victim, NULL, NULL, TO_CHAR );
    act( AT_PINK, "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
    return rNONE;
}

ch_ret spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *ich;

    act( AT_MAGIC, "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You conjure a cloud of purple smoke.", ch, NULL, NULL, TO_CHAR );

    for ( ich = ch->in_room->first_person; ich; ich = ich->next_in_room ) {
        if ( !IS_NPC( ich ) && xIS_SET( ich->act, PLR_WIZINVIS ) )
            continue;

        if ( ich == ch || saves_spell_staff( level, ich ) )
            continue;

        affect_strip( ich, gsn_invis );
        affect_strip( ich, gsn_mass_invis );
        affect_strip( ich, gsn_sneak );
        xREMOVE_BIT( ich->affected_by, AFF_HIDE );
        xREMOVE_BIT( ich->affected_by, AFF_INVISIBLE );
        xREMOVE_BIT( ich->affected_by, AFF_SNEAK );
        act( AT_MAGIC, "$n is revealed!", ich, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "You are revealed!", ich, NULL, NULL, TO_CHAR );
    }
    return rNONE;
}

ch_ret spell_gate( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MOB_INDEX_DATA         *temp;

    if ( ( temp = get_mob_index( MOB_VNUM_VAMPIRE ) ) == NULL ) {
        bug( "Spell_gate: Vampire vnum %d doesn't exist.", MOB_VNUM_VAMPIRE );
        return rSPELL_FAILED;
    }
    char_to_room( create_mobile( temp ), ch->in_room );
    return rNONE;
}

ch_ret spell_harm( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    dam = high;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_identify( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj;

    if ( !VLD_STR( target_name ) ) {
        send_to_char( "What should the spell be cast upon?\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( ( obj = get_obj_carry( ch, target_name ) ) != NULL ) {
        act( AT_PLAIN, "You hold $p and close your eyes.", ch, obj, NULL, TO_CHAR );
        act( AT_MAGIC, "You utter the words, 'Noctull Vaka'.", ch, NULL, NULL, TO_CHAR );
        act( AT_PLAIN, "$n holds $p and closes $s eyes.", ch, obj, NULL, TO_ROOM );
        act( AT_PLAIN, "You feel a flow of knowledge about $p enter your mind.", ch, obj, NULL,
             TO_CHAR );

        identify_object( ch, obj );

        return rNONE;
    }
    else {
        ch_printf( ch, "You can't find %s!\r\n", target_name );
        return rSPELL_FAILED;
    }
    return rNONE;
}

/* -Upcoming darkness spell, -Taon
ch_ret spell_darkness(int sn, int level, CHAR_DATA * ch, void *vo)
{
 

  return rSPELL_FAILED;
}

*/

ch_ret spell_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{

    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

/* Modifications on 1/2/96 to work on player/object - Scryn */

    if ( IS_IMMORTAL( ch ) && ch->level < ( MAX_LEVEL - 1 ) ) {
        send_to_char( "I'm sorry but 6 Dragons staff members must stay visible.\r\n\r\n", ch );
        send_to_char( "Players like to see staff online, and staff like to see players.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( target_name[0] == '\0' )
        victim = ch;
    else
        victim = get_char_room( ch, target_name );

    if ( victim ) {
        AFFECT_DATA             af;

        if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
            immune_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        if ( IS_AFFECTED( victim, AFF_INVISIBLE ) ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        act( AT_MAGIC, "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
        af.type = sn;
        af.duration = ( int ) ( ( ( level / 4 ) + 12 ) * DUR_CONV );
        af.location = APPLY_NONE;
        af.modifier = 0;

        af.level = ch->level;
        af.bitvector = meb( AFF_INVISIBLE );
        affect_to_char( victim, &af );
        act( AT_MAGIC, "You fade out of existence.", victim, NULL, NULL, TO_CHAR );
        return rNONE;
    }
    else {
        OBJ_DATA               *obj;

        obj = get_obj_carry( ch, target_name );

        if ( obj ) {
            separate_obj( obj );                       /* Fix multi-invis bug --Blod */
            if ( IS_OBJ_STAT( obj, ITEM_INVIS ) || chance( ch, 40 + level / 10 ) ) {
                failed_casting( skill, ch, NULL, NULL );
                return rSPELL_FAILED;
            }

            xSET_BIT( obj->extra_flags, ITEM_INVIS );
            act( AT_MAGIC, "$p fades out of existence.", ch, obj, NULL, TO_CHAR );
            return rNONE;
        }
    }
    ch_printf( ch, "You can't find %s!\r\n", target_name );
    return rSPELL_FAILED;
}

ch_ret spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    const char             *msg;
    int                     ap;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( !victim ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    ap = victim->alignment;

    if ( ap > 700 )
        msg = "$N has an aura as white as the driven snow.";
    else if ( ap > 350 )
        msg = "$N is of excellent moral character.";
    else if ( ap > 100 )
        msg = "$N is often kind and thoughtful.";
    else if ( ap > -100 )
        msg = "$N doesn't have a firm moral commitment.";
    else if ( ap > -350 )
        msg = "$N lies to $S friends.";
    else if ( ap > -700 )
        msg = "$N would just as soon kill you as look at you.";
    else
        msg = "$N is as evil as they come!";

    act( AT_MAGIC, msg, ch, NULL, victim, TO_CHAR );
    return rNONE;
}

ch_ret spell_lightning_bolt( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 25, 28,
        31, 34, 37, 40, 40, 41, 42, 42, 43, 44,
        44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
        51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
        58, 58, 59, 60, 60, 61, 62, 62, 63, 64,
        64, 65, 65, 66, 66, 67, 68, 68, 69, 69,
        70, 71, 71, 72, 72, 73, 73, 74, 75, 75
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );

    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        act( AT_YELLOW, "The lightning bolt you cast only partially penetrates $N's shockshield.",
             ch, NULL, victim, TO_CHAR );
        act( AT_YELLOW, "$n's lightning bolt only partially penetrates your shockshield.", ch, NULL,
             victim, TO_VICT );
        dam = number_range( dam_each[level] / 2, dam_each[level] );
    }
    else {
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    }
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo )
{
    char                    buf[MAX_INPUT_LENGTH];
    OBJ_DATA               *obj;
    OBJ_DATA               *in_obj;
    int                     cnt,
                            found = 0;

    if ( !target_name || target_name[0] == '\0' ) {
        send_to_char( "Locate what?\r\n", ch );
        return rSPELL_FAILED;
    }

    for ( obj = first_object; obj; obj = obj->next ) {
        if ( !can_see_obj( ch, obj ) || !nifty_is_name( target_name, obj->name ) )
            continue;
        if ( ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) || IS_OBJ_STAT( obj, ITEM_NOLOCATE ) )
             && !IS_IMMORTAL( ch ) )
            continue;
        if ( obj->in_room && IS_SET( obj->in_room->room_flags, ROOM_CLANSTOREROOM ) )
            continue;

        found++;

        for ( cnt = 0, in_obj = obj; in_obj->in_obj && cnt < 100; in_obj = in_obj->in_obj, ++cnt );
        if ( cnt >= MAX_NEST ) {
            bug( "spell_locate_obj: object [%d] %s is nested more than %d times!",
                 obj->pIndexData->vnum, obj->short_descr, MAX_NEST );
            continue;
        }

        if ( in_obj->carried_by ) {
            if ( IS_IMMORTAL( in_obj->carried_by ) && !IS_NPC( in_obj->carried_by )
                 && ( get_trust( ch ) < in_obj->carried_by->pcdata->wizinvis )
                 && xIS_SET( in_obj->carried_by->act, PLR_WIZINVIS ) ) {
                found--;
                continue;
            }

            snprintf( buf, MAX_INPUT_LENGTH, "%s carried by %s.\r\n", obj_short( obj ),
                      PERS( in_obj->carried_by, ch ) );
        }
        else
            snprintf( buf, MAX_INPUT_LENGTH, "%s in %s.\r\n", obj_short( obj ),
                      in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name );

        buf[0] = UPPER( buf[0] );
        set_char_color( AT_MAGIC, ch );

/*
	Gorog added this 98/09/02 but obj_short(obj) now nukes memory
	and crashes us when the resulting buffer is sent to the pager
	(not confined to just pager_printf).  Something else somewhere
	else must have changed recently to exacerbate this problem,
	as it is now a guaranteed crash cause. - Blodkai
	pager_printf( ch, buf );
*/
        send_to_char( buf, ch );
    }

    if ( !found ) {
        send_to_char( "Nothing like that exists.\r\n", ch );
        return rSPELL_FAILED;
    }
    return rNONE;
}

ch_ret spell_magic_missile( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        3, 3, 4, 4, 5, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
        11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
        13, 13, 13, 13, 13, 14, 14, 14, 14, 14,
        15, 15, 15, 15, 15, 16, 16, 16, 16, 16,
        17, 17, 17, 17, 17, 18, 18, 18, 18, 18
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    /*
     * What's this?  You can't save vs. magic missile!  -Thoric
     * if(saves_spell(level, victim))
     * dam /= 2;
     */
    return damage( ch, victim, dam, sn );
}

ch_ret spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( IS_AFFECTED( victim, AFF_PASS_DOOR ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    af.type = sn;
    af.duration = ( int ) ( number_fuzzy( level / 4 ) * DUR_CONV );
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.level = ch->level;
    af.bitvector = meb( AFF_PASS_DOOR );
    affect_to_char( victim, &af );
    act( AT_MAGIC, "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You turn translucent.", victim, NULL, NULL, TO_CHAR );
    return rNONE;
}

ch_ret spell_iceshard( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_FROSTBITE ) )
        return rNONE;

    af.type = sn;
    af.location = APPLY_STR;
    af.modifier = -6;
    af.duration = 50;
    af.bitvector = meb( AFF_FROSTBITE );
    af.level = 50;
    affect_join( victim, &af );
    return rNONE;
}

ch_ret spell_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    int                     chance;
    short                   speed;
    bool                    first = TRUE;

    speed = number_range( 1, 4 );
    chance = ris_save( victim, level, RIS_POISON );

    if ( saves_poison_death( chance, victim ) && speed > 2 ) {
        send_to_char( "&cYour poison fails to be strong enough to affect them.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( IS_AFFECTED( victim, AFF_POISON ) )
        first = FALSE;
    af.type = sn;
    af.location = APPLY_STR;
    if ( first ) {
        af.duration = level / 2;
        af.modifier = -level / 10;
    }
    else {
        af.modifier = 0;
        af.duration = level / 2;
    }
    if ( IS_SECONDCLASS( ch ) ) {
        victim->degree = 2;
    }
    if ( IS_THIRDCLASS( ch ) ) {
        victim->degree = 3;
    }
    if ( ch->secondclass == -1 ) {
        victim->degree = 1;
    }

    af.bitvector = meb( AFF_POISON );
    af.level = ch->level;
    affect_join( victim, &af );
    set_char_color( AT_GREEN, victim );
    send_to_char( "You feel very sick.\r\n", victim );
    victim->mental_state = URANGE( 5, victim->mental_state + ( first ? 5 : 0 ), 100 );
    if ( ch != victim ) {
        act( AT_GREEN, "$N shivers as your poison spreads through $S body.", ch, NULL, victim,
             TO_CHAR );
        act( AT_GREEN, "$N shivers as $n's poison spreads through $S body.", ch, NULL, victim,
             TO_NOTVICT );
    }
    return rNONE;
}

ch_ret spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( is_affected( victim, gsn_curse ) ) {
        affect_strip( victim, gsn_curse );
        set_char_color( AT_MAGIC, victim );
        send_to_char( "The weight of your curse is lifted.\r\n", victim );
        if ( ch != victim ) {
            act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
        }
    }
    else if ( victim->first_carrying ) {
        for ( obj = victim->first_carrying; obj; obj = obj->next_content )
            if ( !obj->in_obj
                 && ( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) || IS_OBJ_STAT( obj, ITEM_NODROP ) ) ) {
                if ( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
                    xREMOVE_BIT( obj->extra_flags, ITEM_NOREMOVE );
                if ( IS_OBJ_STAT( obj, ITEM_NODROP ) )
                    xREMOVE_BIT( obj->extra_flags, ITEM_NODROP );
                set_char_color( AT_MAGIC, victim );
                send_to_char( "You feel a burden released.\r\n", victim );
                if ( ch != victim ) {
                    act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim,
                         TO_NOTVICT );
                }
                return rNONE;
            }
    }
    return rNONE;
}

ch_ret spell_remove_trap( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj;
    OBJ_DATA               *trap;
    bool                    found;
    int                     retcode;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( !target_name || target_name[0] == '\0' ) {
        send_to_char( "Remove trap on what?\r\n", ch );
        return rSPELL_FAILED;
    }

    found = FALSE;

    if ( !ch->in_room->first_content ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return rNONE;
    }

    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content )
        if ( can_see_obj( ch, obj ) && nifty_is_name( target_name, obj->name ) ) {
            found = TRUE;
            break;
        }

    if ( !found ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( ( trap = get_trap( obj ) ) == NULL ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }

    if ( !chance( ch, 70 + get_curr_wis( ch ) ) ) {
        send_to_char( "Ooops!\r\n", ch );
        retcode = spring_trap( ch, trap );
        if ( retcode == rNONE )
            retcode = rSPELL_FAILED;
        return retcode;
    }

    extract_obj( trap );

    successful_casting( skill, ch, NULL, NULL );
    return rNONE;
}

ch_ret spell_shocking_grasp( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 20, 25, 29, 33,
        36, 39, 39, 39, 40, 40, 41, 41, 42, 42,
        43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
        48, 48, 49, 49, 50, 50, 51, 51, 52, 52,
        53, 53, 54, 54, 55, 55, 56, 56, 57, 57,
        58, 58, 59, 59, 60, 60, 61, 61, 62, 62,
        63, 63, 64, 64, 65, 65, 66, 66, 67, 67
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );

    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        act( AT_YELLOW, "The shocking grasp you cast only partially penetrates $N's shockshield.",
             ch, NULL, victim, TO_CHAR );
        act( AT_YELLOW, "$n's shocking grasp only partially penetrates your shockshield.", ch, NULL,
             victim, TO_VICT );
        dam = number_range( dam_each[level] / 2, dam_each[level] );
    }
    else {
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    }
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    int                     retcode;
    int                     chance;
    int                     tmp;
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_room( ch, target_name ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( victim ) && victim->fighting ) {
        send_to_char( "You cannot sleep a fighting player.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return rSPELL_FAILED;

    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !IS_NPC( ch ) && !IS_NPC( victim ) )
        tmp = level / 2;
    else
        tmp = level;

    if ( IS_AFFECTED( victim, AFF_SLEEP )
         || ( chance = ris_save( victim, tmp, RIS_SLEEP ) ) == 1000 || level < victim->level
         || ( victim != ch && IS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
         || saves_spell_staff( chance, victim ) ) {
        failed_casting( skill, ch, victim, NULL );
        if ( ch == victim )
            return rSPELL_FAILED;
        if ( !victim->fighting ) {
            retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
            if ( retcode == rNONE )
                retcode = rSPELL_FAILED;
            return retcode;
        }
    }
    af.type = sn;
    af.duration = ( int ) ( ( 4 + level ) * DUR_CONV );
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = meb( AFF_SLEEP );
    af.level = ch->level;
    affect_join( victim, &af );

    /*
     * Added by Narn at the request of Dominus. 
     */
    if ( !IS_NPC( victim ) ) {
        char                    buf[MSL];

        snprintf( buf, MSL, "%s has cast sleep on %s.", ch->name, victim->name );
        log_string_plus( buf, LOG_NORMAL, ch->level );
        to_channel( buf, "Monitor", UMAX( LEVEL_IMMORTAL, ch->level ) );
    }

    if ( IS_AWAKE( victim ) ) {
        act( AT_MAGIC, "You feel very sleepy ..... zzzzzz.", victim, NULL, NULL, TO_CHAR );
        act( AT_MAGIC, "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
        set_position( victim, POS_SLEEPING );
    }
    if ( IS_NPC( victim ) )
        start_hating( victim, ch );

    return rNONE;
}

ch_ret spell_summon( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    char                    buf[MSL];
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
         || victim == ch || !victim->in_room || IS_SET( ch->in_room->area->flags, AFLAG_NOSUMMON )
         || IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_SUMMON )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOSUMMON )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || victim->level - ch->level > 6
         || victim->fighting || ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_AGGRESSIVE ) )
         || ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
         || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) )
         || !in_hard_range( victim, ch->in_room->area ) || ( !IS_NPC( ch ) && !IS_NPC( victim )
                                                             && IS_SET( victim->pcdata->flags,
                                                                        PCFLAG_NOSUMMON ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( ch->in_room->area != victim->in_room->area ) {
        if ( ( ( IS_NPC( ch ) != IS_NPC( victim ) ) && chance( ch, 30 ) )
             || ( ( IS_NPC( ch ) == IS_NPC( victim ) ) && chance( ch, 60 ) ) ) {
            failed_casting( skill, ch, victim, NULL );
            set_char_color( AT_MAGIC, victim );
            send_to_char( "You feel a strange pulling sensation...\r\n", victim );
            return rSPELL_FAILED;
        }
    }

    if ( !IS_NPC( ch ) ) {
        act( AT_MAGIC, "You feel a wave of nausea overcome you...", ch, NULL, NULL, TO_CHAR );
        act( AT_MAGIC, "$n collapses, stunned!", ch, NULL, NULL, TO_ROOM );
        set_position( ch, POS_STUNNED );

        snprintf( buf, MSL, "%s summoned %s to room %d.", ch->name, victim->name,
                  ch->in_room->vnum );
        log_string_plus( buf, LOG_NORMAL, ch->level );
        to_channel( buf, "Monitor", UMAX( LEVEL_IMMORTAL, ch->level ) );
    }

    act( AT_MAGIC, "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( AT_MAGIC, "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "$N has summoned you!", victim, NULL, ch, TO_CHAR );
    do_look( victim, ( char * ) "auto" );
    return rNONE;
}

/*
 * Travel via the astral plains to quickly travel to desired location
 *  -Thoric
 *
 * Uses SMAUG spell messages is available to allow use as a SMAUG spell
 */
ch_ret spell_astral_walk( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    struct skill_type      *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL ) {
        send_to_char( "&GThey do not exist.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GYou sense you cannot astral walk at your current location.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GIt is beyond your ability to astral walk to that location.\r\n", ch );
        return rNONE;
    }

    if ( victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&GIt is beyond your ability to astral walk to that location.\r\n", ch );
        return rNONE;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&GYou cannot astral walk to a Staff member.\r\n", ch );
        return rNONE;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&GYou cannot astral to the area that your victim is in.\r\n", ch );
        return rNONE;
    }

    if ( IS_AFFECTED( ch, AFF_SNARE ) || IS_AFFECTED( ch, AFF_PRAYER )
         || IS_AFFECTED( ch, AFF_ROOT ) ) {
        send_to_char( "But you can't move right now.\r\n", ch );
        return rNONE;
    }
    act( AT_YELLOW, "$n disappears in a flash of light!", ch, NULL, NULL, TO_ROOM );
    send_to_char( "&YYou travel through space and time towards your new destination.&D\r\n", ch );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    act( AT_YELLOW, "$n appears in a flash of light!", ch, NULL, NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    return rNONE;
}

ch_ret spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    ROOM_INDEX_DATA        *pRoomIndex;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || ( !IS_NPC( ch ) && victim->fighting ) || ( victim != ch
                                                       && ( saves_spell_staff( level, victim )
                                                            || saves_wands( level, victim ) ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    for ( ;; ) {
        pRoomIndex = get_room_index( number_range( 0, MAX_VNUM ) );
        if ( pRoomIndex )
            if ( !IS_SET( pRoomIndex->room_flags, ROOM_PRIVATE )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_SOLITARY )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_NO_ASTRAL )
                 && !IS_SET( pRoomIndex->area->flags, AFLAG_NOTELEPORT )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_PROTOTYPE )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_NO_RECALL )
                 && in_hard_range( ch, pRoomIndex->area ) )
                break;
    }

    act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    if ( !IS_NPC( victim ) )
        act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
    do_look( victim, ( char * ) "auto" );
    return rNONE;
}

//For use with mana potions... -Taon
ch_ret spell_draw_mana( int sn, int level, CHAR_DATA *ch, void *vo )
{
    int                     amt;

    if ( ch->level < 40 )
        amt = ch->level + ( ( get_curr_wis( ch ) + get_curr_int( ch ) ) * 2 );
    else if ( ch->level < 70 )
        amt = ch->level + ( ( get_curr_wis( ch ) + get_curr_int( ch ) ) * 4 );
    else
        amt = ch->level + ( ( get_curr_wis( ch ) + get_curr_int( ch ) ) * 6 );

    ch->mana += amt;
    send_to_char( "&BA rush of mana fills your body.\r\n", ch );

    if ( ch->mana > ch->max_mana )
        ch->mana = ch->max_mana;

    return rNONE;
}

ch_ret spell_ventriloquate( int sn, int level, CHAR_DATA *ch, void *vo )
{
    char                    buf1[MSL];
    char                    buf2[MSL];
    char                    speaker[MIL];
    CHAR_DATA              *vch;

    target_name = one_argument( target_name, speaker );

    snprintf( buf1, MSL, "%s says '%s'.\r\n", speaker, target_name );
    snprintf( buf2, MSL, "Someone makes %s say '%s'.\r\n", speaker, target_name );
    buf1[0] = UPPER( buf1[0] );

    for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room ) {
        if ( !is_name( speaker, vch->name ) ) {
            set_char_color( AT_SAY, vch );
            send_to_char( saves_spell_staff( level, vch ) ? buf2 : buf1, vch );
        }
    }

    return rNONE;
}

ch_ret spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );

    set_char_color( AT_MAGIC, ch );
    if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    if ( is_affected( victim, sn ) || saves_wands( level, victim ) ) {
        send_to_char( "Your magic fails to take hold.\r\n", ch );
        return rSPELL_FAILED;
    }
    af.type = sn;
    af.duration = level + 10;
    af.location = APPLY_STR;
    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        af.modifier = -4;
    }
    else {
        af.modifier = -2;
    }
    af.level = ch->level;
    xCLEAR_BITS( af.bitvector );
    affect_to_char( victim, &af );
    set_char_color( AT_MAGIC, victim );
    send_to_char( "Your muscles seem to atrophy!\r\n", victim );
    if ( ch != victim ) {
        if ( ( ( ( !IS_NPC( victim ) && class_table[victim->Class]->attr_prime == APPLY_STR )
                 || IS_NPC( victim ) ) && get_curr_str( victim ) < 25 )
             || get_curr_str( victim ) < 20 ) {
            act( AT_MAGIC, "$N labors weakly as your spell atrophies $S muscles.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_MAGIC, "$N labors weakly as $n's spell atrophies $S muscles.", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else {
            act( AT_MAGIC, "You induce a mild atrophy in $N's muscles.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_MAGIC, "$n induces a mild atrophy in $N's muscles.", ch, NULL, victim,
                 TO_NOTVICT );
        }
    }
    return rNONE;
}

ch_ret spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo )
{

    act( AT_MAGIC, "A spiral of glowing energy encompasses you, and you recall.", ch, NULL, NULL,
         TO_CHAR );
    act( AT_MAGIC, "$n is encompassed by a spiral of glowing energy, and recalls!", ch, NULL, NULL,
         TO_ROOM );
    do_recall( ( CHAR_DATA * ) vo, ( char * ) "" );
    return rNONE;
}

/*
 * NPC spells.
 */
ch_ret spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    OBJ_DATA               *obj_lose;
    OBJ_DATA               *obj_next;
    int                     dam;

    if ( chance( ch, 2 * level ) && !saves_breath( level, victim ) ) {
        for ( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next ) {
            int                     iWear;

            obj_next = obj_lose->next_content;

            if ( number_bits( 2 ) != 0 )
                continue;

            switch ( obj_lose->item_type ) {
                case ITEM_ARMOR:
                    if ( obj_lose->value[0] > 0 ) {
                        separate_obj( obj_lose );
                        act( AT_DAMAGE, "$p is pitted and etched!", victim, obj_lose, NULL,
                             TO_CHAR );
                        if ( ( iWear = obj_lose->wear_loc ) != WEAR_NONE )
                            victim->armor += apply_ac( obj_lose, iWear );
                        obj_lose->value[0] -= 1;
                        obj_lose->cost = 0;
                        if ( iWear != WEAR_NONE )
                            victim->armor -= apply_ac( obj_lose, iWear );
                    }
                    break;

                case ITEM_CONTAINER:
                    separate_obj( obj_lose );
                    act( AT_DAMAGE, "$p fumes and dissolves!", victim, obj_lose, NULL, TO_CHAR );
                    act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim,
                         obj_lose, victim, TO_ROOM );
                    act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim,
                         obj_lose, NULL, TO_CHAR );
                    empty_obj( obj_lose, NULL, victim->in_room, FALSE );
                    extract_obj( obj_lose );
                    break;
            }
        }
    }
    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        dam = insane;
    }
    else {
        dam = ludicrous;
    }
    if ( saves_breath( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    OBJ_DATA               *obj_lose;
    OBJ_DATA               *obj_next;
    int                     dam;

    if ( chance( ch, 2 * level ) && !saves_breath( level, victim ) ) {
        for ( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next ) {
            const char             *msg;

            obj_next = obj_lose->next_content;
            if ( number_bits( 2 ) != 0 )
                continue;

            switch ( obj_lose->item_type ) {
                default:
                    continue;
                case ITEM_CONTAINER:
                    msg = "$p ignites and burns!";
                    break;
                case ITEM_POTION:
                    msg = "$p bubbles and boils!";
                    break;
                case ITEM_SCROLL:
                    msg = "$p crackles and burns!";
                    break;
                case ITEM_STAFF:
                    msg = "$p smokes and chars!";
                    break;
                case ITEM_WAND:
                    msg = "$p sparks and sputters!";
                    break;
                case ITEM_COOK:
                case ITEM_FOOD:
                    msg = "$p blackens and crisps!";
                    break;
                case ITEM_PILL:
                    msg = "$p melts and drips!";
                    break;
            }

            separate_obj( obj_lose );
            act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
            if ( obj_lose->item_type == ITEM_CONTAINER ) {
                act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim,
                     obj_lose, victim, TO_ROOM );
                act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose,
                     NULL, TO_CHAR );
                empty_obj( obj_lose, NULL, victim->in_room, FALSE );
            }
            extract_obj( obj_lose );
        }
    }
    dam = ludicrous;
    if ( saves_breath( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    OBJ_DATA               *obj_lose;
    OBJ_DATA               *obj_next;
    int                     dam;

    if ( chance( ch, 2 * level ) && !saves_breath( level, victim ) ) {
        for ( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next ) {
            const char             *msg;

            obj_next = obj_lose->next_content;
            if ( number_bits( 2 ) != 0 )
                continue;

            switch ( obj_lose->item_type ) {
                default:
                    continue;
                case ITEM_CONTAINER:
                case ITEM_DRINK_CON:
                case ITEM_POTION:
                    msg = "$p freezes and shatters!";
                    break;
            }

            separate_obj( obj_lose );
            act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
            if ( obj_lose->item_type == ITEM_CONTAINER ) {
                act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim,
                     obj_lose, victim, TO_ROOM );
                act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose,
                     NULL, TO_CHAR );
                empty_obj( obj_lose, NULL, victim->in_room, FALSE );
            }
            extract_obj( obj_lose );
        }
    }
    dam = ludicrous;

    if ( saves_breath( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    int                     dam;
    bool                    ch_died;

    ch_died = FALSE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "You fail to breathe.\r\n", ch );
        return rNONE;
    }

    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
        vch_next = vch->next_in_room;
        if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
             && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
            continue;

        if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
            continue;

        if ( is_same_group( vch, ch ) )
            continue;
        if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) || ( in_arena( vch ) && vch != ch )
             || ( IS_PKILL( vch ) && vch != ch ) ) {
            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
                dam = insane;
            else
                dam = ludicrous;
            if ( saves_breath( level, vch ) )
                dam /= 2;
            if ( damage( ch, vch, dam, sn ) == rCHAR_DIED || char_died( ch ) )
                ch_died = TRUE;
        }
    }
    if ( ch_died )
        return rCHAR_DIED;
    else
        return rNONE;
}

ch_ret spell_lightning_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    dam = ludicrous;

    if ( saves_breath( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

/* Working on DM's transport eq suggestion - Scryn 8/13 */
ch_ret spell_transport( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    char                    arg3[MSL];
    OBJ_DATA               *obj;
    SKILLTYPE              *skill = get_skilltype( sn );

    target_name = one_argument( target_name, arg3 );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
         || victim == ch || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_DEATH )
         || IS_SET( victim->in_room->room_flags, ROOM_PROTOTYPE )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
         || victim->level >= level + 15 || ( IS_NPC( victim )
                                             && xIS_SET( victim->act, ACT_PROTOTYPE ) )
         || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( victim->in_room == ch->in_room ) {
        send_to_char( "They are right beside you!", ch );
        return rSPELL_FAILED;
    }

    if ( ( obj = get_obj_carry( ch, arg3 ) ) == NULL
         || ( victim->carry_weight + get_obj_weight( obj, FALSE ) ) > can_carry_w( victim )
         || ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    separate_obj( obj );                               /* altrag shoots, haus alley-oops! 
                                                        */

    if ( IS_OBJ_STAT( obj, ITEM_NODROP ) ) {
        send_to_char( "You can't seem to let go of it.\r\n", ch );
        return rSPELL_FAILED;                          /* nice catch, caine */
    }

    if ( IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) ) {
        send_to_char( "You can't seem to let go of it.\r\n", ch );
        return rSPELL_FAILED;                          /* nice catch, caine */
    }

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "That item is not for mortal hands to touch!\r\n", ch );
        return rSPELL_FAILED;                          /* Thoric */
    }

    act( AT_MAGIC, "$p slowly dematerializes...", ch, obj, NULL, TO_CHAR );
    act( AT_MAGIC, "$p slowly dematerializes from $n's hands..", ch, obj, NULL, TO_ROOM );
    obj_from_char( obj );
    obj_to_char( obj, victim );
    act( AT_MAGIC, "$p from $n appears in your hands!", ch, obj, victim, TO_VICT );
    act( AT_MAGIC, "$p appears in $n's hands!", victim, obj, NULL, TO_ROOM );
    save_char_obj( ch );
    save_char_obj( victim );
    return rNONE;
}

/*
 * Syntax portal (mob/char) 
 * opens a 2-way EX_PORTAL from caster's room to room inhabited by  
 *  mob or character won't mess with existing exits
 *
 * do_mp_open_passage, combined with spell_astral
 */
ch_ret spell_portal( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    ROOM_INDEX_DATA        *targetRoom,
                           *fromRoom;
    int                     targetRoomVnum;
    OBJ_DATA               *portalObj;
    EXIT_DATA              *pexit;
    char                    buf[MSL];
    SKILLTYPE              *skill = get_skilltype( sn );

    /*
     * No go if all kinds of things aren't just right, including the caster
     * and victim are not both pkill or both peaceful. -- Narn
     */

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL ) {
        send_to_char( "&GThey do not exist.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GYou sense you cannot portal at your current location.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GIt is beyond your ability to portal to that location.\r\n", ch );
        return rNONE;
    }

    if ( victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&GIt is beyond your ability to portal to that location.\r\n", ch );
        return rNONE;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&GYou cannot portal to a Staff member.\r\n", ch );
        return rNONE;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&GYou cannot portal to the area that your victim is in.\r\n", ch );
        return rNONE;
    }

    targetRoomVnum = victim->in_room->vnum;
    fromRoom = ch->in_room;
    targetRoom = victim->in_room;

    /*
     * Check if there already is a portal in either room. 
     */
    for ( pexit = fromRoom->first_exit; pexit; pexit = pexit->next ) {
        if ( IS_SET( pexit->exit_info, EX_PORTAL ) ) {
            send_to_char( "There is already a portal in this room.\r\n", ch );
            return rSPELL_FAILED;
        }

        if ( pexit->vdir == DIR_PORTAL ) {
            send_to_char( "You may not create a portal in this room.\r\n", ch );
            return rSPELL_FAILED;
        }
    }

    for ( pexit = targetRoom->first_exit; pexit; pexit = pexit->next )
        if ( pexit->vdir == DIR_PORTAL ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

    pexit = make_exit( fromRoom, targetRoom, DIR_PORTAL );
    pexit->keyword = STRALLOC( "portal" );
    pexit->description = STRALLOC( "You gaze into the shimmering portal...\r\n" );
    pexit->key = -1;
    pexit->exit_info = EX_PORTAL | EX_xENTER | EX_HIDDEN | EX_xLOOK;
    pexit->vnum = targetRoomVnum;

    portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    portalObj->timer = 3;
    snprintf( buf, MSL, "a magical portal" );
    STRFREE( portalObj->short_descr );
    portalObj->short_descr = STRALLOC( buf );
    STRFREE( portalObj->description );
    portalObj->description =
        STRALLOC( "&ZA whirling portal of energy turns slowly, beckoning to the unknown." );

    /*
     * support for new casting messages 
     */
    if ( !skill->hit_char || skill->hit_char[0] == '\0' ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "You utter an incantation, and a portal forms in front of you!\r\n", ch );
    }
    else
        act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
    if ( !skill->hit_room || skill->hit_room[0] == '\0' )
        act( AT_MAGIC, "$n utters an incantation, and a portal forms in front of you!", ch, NULL,
             NULL, TO_ROOM );
    else
        act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
    if ( !skill->hit_vict || skill->hit_vict[0] == '\0' )
        act( AT_MAGIC, "A shimmering portal forms in front of you!", victim, NULL, NULL, TO_ROOM );
    else
        act( AT_MAGIC, skill->hit_vict, victim, NULL, victim, TO_ROOM );
    portalObj = obj_to_room( portalObj, ch->in_room );
    pexit = make_exit( targetRoom, fromRoom, DIR_PORTAL );
    pexit->keyword = STRALLOC( "portal" );
    pexit->description = STRALLOC( "You gaze into the shimmering portal...\r\n" );
    pexit->key = -1;
    pexit->exit_info = EX_PORTAL | EX_xENTER | EX_HIDDEN;
    pexit->vnum = targetRoomVnum;

    portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    portalObj->timer = 3;
    STRFREE( portalObj->short_descr );
    portalObj->short_descr = STRALLOC( buf );
    STRFREE( portalObj->description );
    portalObj->description =
        STRALLOC( "&ZA whirling portal of energy turns slowly, beckoning to the unknown." );
    portalObj = obj_to_room( portalObj, targetRoom );
    act( AT_MAGIC, "A shimmering portal forms in front of you!", ch, NULL, victim, TO_VICT );
    return rNONE;
}

ch_ret spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo )
{
    ROOM_INDEX_DATA        *location;
    ROOM_INDEX_DATA        *original;
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    /*
     * The spell fails if the victim isn't playing, the victim is the caster,
     * the target room has private, solitary, noastral, death or proto flags,
     * the caster's room is norecall, the victim is too high in level, the 
     * victim is a proto mob, the victim makes the saving throw or the pkill 
     * flag on the caster is not the same as on the victim.  Got it?
     */
    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
         || victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_DEATH )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_PROTOTYPE )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) || ( IS_NPC( victim )
                                                                   && xIS_SET( victim->act,
                                                                               ACT_PROTOTYPE ) )
         || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    location = victim->in_room;
    if ( !location ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    successful_casting( skill, ch, victim, NULL );
    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    do_look( ch, ( char * ) "auto" );
    char_from_room( ch );
    char_to_room( ch, original );
    return rNONE;
}

ch_ret spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;

    if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND ) {
        separate_obj( obj );
        if ( obj->value[2] == obj->value[1] || obj->value[1] > ( obj->pIndexData->value[1] * 4 ) ) {
            act( AT_FIRE, "$p bursts into flames, injuring you!", ch, obj, NULL, TO_CHAR );
            act( AT_FIRE, "$p bursts into flames, charring $n!", ch, obj, NULL, TO_ROOM );
            extract_obj( obj );
            if ( damage( ch, ch, obj->level * 2, TYPE_UNDEFINED ) == rCHAR_DIED || char_died( ch ) )
                return rCHAR_DIED;
            else
                return rSPELL_FAILED;
        }

        if ( chance( ch, 2 ) ) {
            act( AT_YELLOW, "$p glows with a blinding magical luminescence.", ch, obj, NULL,
                 TO_CHAR );
            obj->value[1] *= 2;
            obj->value[2] = obj->value[1];
            return rNONE;
        }
        else if ( chance( ch, 5 ) ) {
            act( AT_YELLOW, "$p glows brightly for a few seconds...", ch, obj, NULL, TO_CHAR );
            obj->value[2] = obj->value[1];
            return rNONE;
        }
        else if ( chance( ch, 10 ) ) {
            act( AT_WHITE, "$p disintegrates into a void.", ch, obj, NULL, TO_CHAR );
            act( AT_WHITE, "$n's attempt at recharging fails, and $p disintegrates.", ch, obj, NULL,
                 TO_ROOM );
            extract_obj( obj );
            return rSPELL_FAILED;
        }
        else if ( chance( ch, 50 - ( ch->level / 2 ) ) ) {
            send_to_char( "Nothing happens.\r\n", ch );
            return rSPELL_FAILED;
        }
        else {
            act( AT_MAGIC, "$p feels warm to the touch.", ch, obj, NULL, TO_CHAR );
            --obj->value[1];
            obj->value[2] = obj->value[1];
            return rNONE;
        }
    }
    else {
        send_to_char( "You can't recharge that!\r\n", ch );
        return rSPELL_FAILED;
    }
}

/*
 * Idea from AD&D 2nd edition player's handbook (c)1989 TSR Hobbies Inc.
 * -Thoric
 */
ch_ret spell_plant_pass( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
         || victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_DEATH )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_PROTOTYPE )
         || ( victim->in_room->sector_type != SECT_FOREST
              && victim->in_room->sector_type != SECT_FIELD )
         || ( ch->in_room->sector_type != SECT_FOREST && ch->in_room->sector_type != SECT_FIELD )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) || ( IS_NPC( victim )
                                                                   && xIS_SET( victim->act,
                                                                               ACT_PROTOTYPE ) )
         || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    if ( ch->in_room->sector_type == SECT_FOREST )
        act( AT_MAGIC, "$n melds into a nearby tree!", ch, NULL, NULL, TO_ROOM );
    else
        act( AT_MAGIC, "$n melds into the grass!", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    if ( ch->in_room->sector_type == SECT_FOREST )
        act( AT_MAGIC, "$n appears from behind a nearby tree!", ch, NULL, NULL, TO_ROOM );
    else
        act( AT_MAGIC, "$n grows up from the grass!", ch, NULL, NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    return rNONE;
}

/*
 * Vampire version of astral_walk     -Thoric
 */
ch_ret spell_mist_walk( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    set_char_color( AT_DGREEN, ch );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL || victim == ch ) {
        failed_casting( skill, ch, victim, NULL );
        send_to_char( "You cannot sense your victim...", ch );
        return rSPELL_FAILED;
    }

    if ( !victim->in_room || ch->blood < 24 || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_DEATH )
         || IS_SET( victim->in_room->room_flags, ROOM_PROTOTYPE ) || ( IS_NPC( victim )
                                                                       && xIS_SET( victim->act,
                                                                                   ACT_PROTOTYPE ) ) )
    {
        failed_casting( skill, ch, victim, NULL );
        send_to_char( "You cannot sense your victim, or are too weak on blood.", ch );
        return rSPELL_FAILED;
    }

    /*
     * Subtract 22 extra bp for mist walk from 0500 to 2100 SB 
     */
    if ( time_info.hour < 21 && time_info.hour > 5 && !IS_NPC( ch ) )
        ch->blood = ch->blood - 22;

    act( AT_DGREEN, "$n dissolves into a cloud of glowing mist, then vanishes!", ch, NULL, NULL,
         TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    act( AT_DGREEN, "A cloud of glowing mist engulfs you, then withdraws to unveil $n!", ch, NULL,
         NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    return rNONE;
}

/*
 * Cleric version of astral_walk     -Thoric
 */
ch_ret spell_solar_flight( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL ) {
        send_to_char( "&GThey do not exist.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GYou sense you cannot solar flight at your current location.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GIt is beyond your ability to solar flight to that location.\r\n", ch );
        return rNONE;
    }

    if ( victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&GIt is beyond your ability to solar flight to that location.\r\n", ch );
        return rNONE;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&GYou cannot solar flight to a Staff member.\r\n", ch );
        return rNONE;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&GYou cannot solar flight to the area that your victim is in.\r\n", ch );
        return rNONE;
    }

    act( AT_MAGIC, "$n disappears in a blinding flash of light!", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    act( AT_MAGIC, "$n appears in a blinding flash of light!", ch, NULL, NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    return rNONE;
}

/* Scryn 2/2/96 */
ch_ret spell_remove_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( target_name[0] == '\0' ) {
        send_to_char( "What should the spell be cast upon?\r\n", ch );
        return rSPELL_FAILED;
    }

    obj = get_obj_carry( ch, target_name );

    if ( obj ) {
        if ( !IS_OBJ_STAT( obj, ITEM_INVIS ) ) {
            send_to_char( "Its not invisible!\r\n", ch );
            return rSPELL_FAILED;
        }

        xREMOVE_BIT( obj->extra_flags, ITEM_INVIS );
        act( AT_MAGIC, "$p becomes visible again.", ch, obj, NULL, TO_CHAR );

        return rNONE;
    }
    else {
        CHAR_DATA              *victim;

        victim = get_char_room( ch, target_name );

        if ( victim ) {
            if ( !can_see( ch, victim ) ) {
                ch_printf( ch, "You don't see %s!\r\n", target_name );
                return rSPELL_FAILED;
            }

            if ( !IS_AFFECTED( victim, AFF_INVISIBLE ) ) {
                if ( ch == victim )
                    send_to_char( "You are not invisible!\r\n", ch );
                else
                    send_to_char( "They are not invisible!\r\n", ch );
                return rSPELL_FAILED;
            }

            if ( is_safe( ch, victim, TRUE ) ) {
                failed_casting( skill, ch, victim, NULL );
                return rSPELL_FAILED;
            }

            if ( IS_SET( victim->immune, RIS_MAGIC ) ) {
                immune_casting( skill, ch, victim, NULL );
                return rSPELL_FAILED;
            }
            if ( !IS_NPC( victim ) ) {
                if ( chance( ch, 50 ) && ch->level + 10 < victim->level ) {
                    failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
                else
                    check_illegal_pk( ch, victim );
            }
            else {
                if ( chance( ch, 50 ) && ch->level + 15 < victim->level ) {
                    failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
            }

            affect_strip( victim, gsn_invis );
            affect_strip( victim, gsn_mass_invis );
            xREMOVE_BIT( victim->affected_by, AFF_INVISIBLE );
            successful_casting( skill, ch, victim, NULL );
            return rNONE;
        }

        ch_printf( ch, "You can't find %s!\r\n", target_name );
        return rSPELL_FAILED;
    }
}

/*
 * Animate Dead: Scryn 3/2/96
 * Modifications by Altrag 16/2/96
 * Coded and fixed by Volk
 */
ch_ret spell_animate_skeleton( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *mob;
    OBJ_DATA               *corpse;
    OBJ_DATA               *corpse_next;
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    bool                    found;
    MOB_INDEX_DATA         *pMobIndex;
    AFFECT_DATA             af;
    char                    buf[MSL];
    SKILLTYPE              *skill = get_skilltype( sn );

    found = FALSE;

    for ( corpse = ch->in_room->first_content; corpse; corpse = corpse_next ) {
        corpse_next = corpse->next_content;

        if ( corpse->item_type == ITEM_SKELETON ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "You cannot find a suitable skeleton here.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( get_mob_index( MOB_VNUM_ANIMATED_SKEL ) == NULL ) {
        bug( "%s", "Vnum 6 not found for spell_animate_skeleton!" );
        return rNONE;
    }

    if ( ( pMobIndex = get_mob_index( ( int ) abs( corpse->cost ) ) ) == NULL ) {
        bug( "%s", "Can not find mob for cost of corpse, spell_animate_skeleton" );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_BLOODCLASS( ch ) ) {
            if ( !IS_IMMORTAL( ch ) && ch->blood - ( pMobIndex->level / 2 ) < 0 ) {
                send_to_char( "You do not have enough blood power to reanimate this"
                              " skeleton.\r\n", ch );
                return rSPELL_FAILED;
            }
            ch->blood = ch->blood - pMobIndex->level / 2;
        }
        else if ( ch->mana - ( pMobIndex->level * 2 ) < 0 ) {
            send_to_char( "You do not have enough mana to reanimate this " "skeleton.\r\n", ch );
            return rSPELL_FAILED;
        }
        else
            ch->mana -= ( pMobIndex->level * 2 );
    }

    if ( IS_IMMORTAL( ch ) || ( chance( ch, 75 ) && pMobIndex->level - ch->level < 10 ) ) {
        mob = create_mobile( get_mob_index( MOB_VNUM_ANIMATED_SKEL ) );
        char_to_room( mob, ch->in_room );
        mob->level = UMIN( ch->level / 2, pMobIndex->level );
        mob->race = pMobIndex->race;                   /* should be undead */

        /*
         * Fix so mobs wont have 0 hps and crash mud - Scryn 2/20/96 
         */
        if ( !pMobIndex->hitnodice )
            mob->max_hit =
                pMobIndex->level * 6 + number_range( pMobIndex->level * pMobIndex->level / 4,
                                                     pMobIndex->level * pMobIndex->level );
        else
            mob->max_hit =
                dice( pMobIndex->hitnodice, pMobIndex->hitsizedice ) + pMobIndex->hitplus;
        mob->max_hit =
            UMAX( URANGE
                  ( mob->max_hit / 4, ( mob->max_hit * corpse->value[3] ) / 100,
                    ch->level * dice( 20, 10 ) ), 1 );

        mob->hit = mob->max_hit;
        mob->damroll = ch->level / 10;
        mob->hitroll = ch->level / 8;
        mob->alignment = ch->alignment;

        act( AT_MAGIC, "$n's magic stirs forgotten life in $T!", ch, NULL, pMobIndex->short_descr,
             TO_ROOM );
        act( AT_MAGIC, "Your magic causes $T to stir, and rise up!", ch, NULL,
             pMobIndex->short_descr, TO_CHAR );

        snprintf( buf, MSL, "animated skeleton %s", pMobIndex->player_name );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );

        snprintf( buf, MSL, "the animated skeleton of %s", pMobIndex->short_descr );
        STRFREE( mob->short_descr );
        mob->short_descr = STRALLOC( buf );

        snprintf( buf, MSL,
                  "An animated skeleton of %s swaggers mechanically, it's skull fixed in a toothy grin.\r\n",
                  pMobIndex->short_descr );
        STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        add_follower( mob, ch );
        af.type = sn;
        af.duration = ( int ) ( ( number_fuzzy( ( level + 1 ) / 4 ) + 1 ) * DUR_CONV );
        af.location = 0;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_CHARM );
        affect_to_char( mob, &af );

        if ( corpse->first_content )
            for ( obj = corpse->first_content; obj; obj = obj_next ) {
                obj_next = obj->next_content;
                obj_from_obj( obj );
                obj_to_room( obj, corpse->in_room );
            }

        separate_obj( corpse );
        extract_obj( corpse );
        return rNONE;
    }
    else {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
}

ch_ret spell_animate_corpse( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *mob;
    OBJ_DATA               *corpse;
    OBJ_DATA               *corpse_next;
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    bool                    found;
    MOB_INDEX_DATA         *pMobIndex;
    AFFECT_DATA             af;
    char                    buf[MSL];
    SKILLTYPE              *skill = get_skilltype( sn );

    found = FALSE;

    for ( corpse = ch->in_room->first_content; corpse; corpse = corpse_next ) {
        corpse_next = corpse->next_content;

        if ( corpse->item_type == ITEM_CORPSE_NPC && corpse->cost != -5 && corpse->value[5] < 1 ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "You cannot find a suitable corpse here.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( get_mob_index( MOB_VNUM_ANIMATED_CORPSE ) == NULL ) {
        bug( "%s", "Vnum 5 not found for spell_animate_corpse!" );
        return rNONE;
    }

    if ( ( pMobIndex = get_mob_index( ( int ) abs( corpse->cost ) ) ) == NULL ) {
        bug( "%s", "Can not find mob for cost of corpse, spell_animate_corpse" );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_BLOODCLASS( ch ) ) {
            if ( !IS_IMMORTAL( ch ) && ch->blood - ( pMobIndex->level / 4 ) < 0 ) {
                send_to_char( "You do not have enough blood power to reanimate this" " corpse.\r\n",
                              ch );
                return rSPELL_FAILED;
            }
            ch->blood = ch->blood - pMobIndex->level / 3;
        }
        else if ( ch->mana - ( pMobIndex->level * 4 ) < 0 ) {
            send_to_char( "You do not have enough mana to reanimate this " "corpse.\r\n", ch );
            return rSPELL_FAILED;
        }
        else
            ch->mana -= ( pMobIndex->level * 4 );
    }

    if ( IS_IMMORTAL( ch ) || ( chance( ch, 75 ) && pMobIndex->level - ch->level < 10 ) ) {
        mob = create_mobile( get_mob_index( MOB_VNUM_ANIMATED_CORPSE ) );
        char_to_room( mob, ch->in_room );
        mob->level = UMIN( ch->level / 2, pMobIndex->level );
        mob->race = pMobIndex->race;                   /* should be undead */

        /*
         * Fix so mobs wont have 0 hps and crash mud - Scryn 2/20/96
         */
        if ( !pMobIndex->hitnodice )
            mob->max_hit =
                pMobIndex->level * 10 + number_range( pMobIndex->level * pMobIndex->level / 4,
                                                      pMobIndex->level * pMobIndex->level );
        else
            mob->max_hit =
                dice( pMobIndex->hitnodice, pMobIndex->hitsizedice ) + pMobIndex->hitplus;
        mob->max_hit =
            UMAX( URANGE
                  ( mob->max_hit / 4, ( mob->max_hit * corpse->value[3] ) / 100,
                    ch->level * dice( 20, 15 ) ), 1 );

        mob->hit = mob->max_hit;
        mob->damroll = ch->level / 6;
        mob->hitroll = ch->level / 4;
        mob->alignment = ch->alignment;

        act( AT_MAGIC, "$n makes $T rise from the grave!", ch, NULL, pMobIndex->short_descr,
             TO_ROOM );
        act( AT_MAGIC, "You make $T rise from the grave!", ch, NULL, pMobIndex->short_descr,
             TO_CHAR );

        snprintf( buf, MSL, "animated corpse %s", pMobIndex->player_name );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );

        snprintf( buf, MSL, "the animated corpse of %s", pMobIndex->short_descr );
        STRFREE( mob->short_descr );
        mob->short_descr = STRALLOC( buf );

        snprintf( buf, MSL,
                  "An animated corpse of %s struggles with the horror of its undeath.\r\n",
                  pMobIndex->short_descr );
        STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        add_follower( mob, ch );
        af.type = sn;
        af.duration = ( int ) ( ( number_fuzzy( ( level + 1 ) / 4 ) + 1 ) * DUR_CONV );
        af.location = 0;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_CHARM );
        affect_to_char( mob, &af );

        if ( corpse->first_content )
            for ( obj = corpse->first_content; obj; obj = obj_next ) {
                obj_next = obj->next_content;
                obj_from_obj( obj );
                obj_to_room( obj, corpse->in_room );
            }

        separate_obj( corpse );
        extract_obj( corpse );
        return rNONE;
    }
    else {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
}

/* Works now.. -- Altrag */
ch_ret spell_possess( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    SKILLTYPE              *skill = get_skilltype( sn );
    int                     ctry;

    if ( !ch->desc || ch->desc->original ) {
        send_to_char( "You are not in your original state.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( ( victim = get_char_room( ch, target_name ) ) == NULL ) {
        send_to_char( "They aren't here!\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( victim == ch ) {
        send_to_char( "You can't possess yourself!\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "You can't possess another player!\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( victim->desc ) {
        ch_printf( ch, "%s is already possessed.\r\n", victim->short_descr );
        return rSPELL_FAILED;
    }

    if ( IS_SET( victim->immune, RIS_MAGIC ) || IS_SET( victim->immune, RIS_CHARM ) ) {
        immune_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    ctry = ris_save( victim, level, RIS_CHARM );

    if ( IS_AFFECTED( victim, AFF_POSSESS ) || IS_AFFECTED( ch, AFF_CHARM ) || level < victim->level
         || victim->desc || saves_spell_staff( ctry, victim ) || !chance( ch, 25 ) ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    af.type = sn;
    af.duration = 20 + ( ch->level - victim->level ) / 2;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = meb( AFF_POSSESS );
    affect_to_char( victim, &af );

    ch_printf( ch, "You have possessed %s!\r\n", victim->short_descr );

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    ch->switched = victim;

    return rNONE;
}

/* Ignores pickproofs, but can't unlock containers. -- Altrag 17/2/96 */
ch_ret spell_knock( int sn, int level, CHAR_DATA *ch, void *vo )
{
    EXIT_DATA              *pexit;
    SKILLTYPE              *skill = get_skilltype( sn );

    set_char_color( AT_MAGIC, ch );
    /*
     * shouldn't know why it didn't work, and shouldn't work on pickproof
     * exits.  -Thoric
     */
    if ( !( pexit = find_door( ch, target_name, FALSE ) ) || !IS_SET( pexit->exit_info, EX_CLOSED )
         || !IS_SET( pexit->exit_info, EX_LOCKED ) || IS_SET( pexit->exit_info, EX_PICKPROOF ) ) {
        OBJ_DATA               *obj =
            get_obj_list_rev( ch, target_name, ch->in_room->last_content );

        if ( obj )
            send_to_char( "Knock can only be cast on doors (exits)!\r\n", ch );
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    REMOVE_BIT( pexit->exit_info, EX_LOCKED );
    send_to_char( "*Click*\r\n", ch );
    if ( pexit->rexit && pexit->rexit->to_room == ch->in_room )
        REMOVE_BIT( pexit->rexit->exit_info, EX_LOCKED );
    check_room_for_traps( ch, TRAP_UNLOCK | trap_door[pexit->vdir] );
    return rNONE;
}

/* Tells to sleepers in area. -- Altrag 17/2/96 */
ch_ret spell_dream( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    target_name = one_argument( target_name, arg );
    set_char_color( AT_MAGIC, ch );
    if ( !( victim = get_char_world( ch, arg ) ) || victim->in_room->area != ch->in_room->area ) {
        send_to_char( "They aren't here.\r\n", ch );
        return rSPELL_FAILED;
    }
    if ( victim->position != POS_SLEEPING ) {
        send_to_char( "They aren't asleep.\r\n", ch );
        return rSPELL_FAILED;
    }
    if ( !target_name ) {
        send_to_char( "What do you want them to dream about?\r\n", ch );
        return rSPELL_FAILED;
    }

    set_char_color( AT_TELL, victim );
    ch_printf( victim, "You have dreams about %s telling you '%s'.\r\n", PERS( ch, victim ),
               target_name );
    successful_casting( get_skilltype( sn ), ch, victim, NULL );
    return rNONE;
}

/* Added spells spiral_blast, scorching surge,
    nostrum, and astral   by SB for Augurer class 
7/10/96 */
//modified -Taon
ch_ret spell_spiral_blast( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    bool                    ch_died;

    ch_died = FALSE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "You fail to breathe.\r\n", ch );
        return rNONE;
    }

    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
        vch_next = vch->next_in_room;
        if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
             && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
            continue;

        if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
            continue;

        if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) {
            act( AT_MAGIC, "Swirling colours radiate from $n" ", encompassing $N.", ch, ch, vch,
                 TO_ROOM );
            act( AT_MAGIC, "Swirling colours radiate from you," " encompassing $N", ch, ch, vch,
                 TO_CHAR );

            if ( damage( ch, vch, medium, sn ) == rCHAR_DIED || char_died( ch ) )
                ch_died = TRUE;
        }
    }

    if ( ch_died )
        return rCHAR_DIED;
    else
        return rNONE;
}

ch_ret spell_scorching_surge( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    static const short      dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 30, 35, 40, 45, 50, 55,
        60, 65, 70, 75, 80, 82, 84, 86, 88, 90,
        92, 94, 96, 98, 100, 102, 104, 106, 108, 110,
        112, 114, 116, 118, 120, 122, 124, 126, 128, 130,
        132, 134, 136, 138, 140, 142, 144, 146, 148, 150,
        152, 154, 156, 158, 160, 162, 164, 166, 168, 170
    };
    int                     dam;

    level = UMIN( ( unsigned int ) level, sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    act( AT_MAGIC, "A fiery current lashes through $n's body!", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "A fiery current lashes through your body!", ch, NULL, NULL, TO_CHAR );
    return damage( ch, victim, ( int ) ( dam * 1.4 ), sn );
}

ch_ret spell_helical_flow( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL ) {
        send_to_char( "&GThey do not exist.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GYou sense you cannot helical flow at your current location.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GIt is beyond your ability to helical flow to that location.\r\n", ch );
        return rNONE;
    }

    if ( victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&GIt is beyond your ability to helical flow to that location.\r\n", ch );
        return rNONE;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&GYou cannot helical flow to a Staff member.\r\n", ch );
        return rNONE;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&GYou cannot helical flow to the area that your victim is in.\r\n", ch );
        return rNONE;
    }

    act( AT_MAGIC, "$n coils into an ascending column of colour," " vanishing into thin air.", ch,
         NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    act( AT_MAGIC, "A coil of colours descends from above, " "revealing $n as it dissipates.", ch,
         NULL, NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    return rNONE;
}

  /*******************************************************
   * Everything after this point is part of SMAUG SPELLS *
   *******************************************************/

/*
 * saving throw check      -Thoric
 */
bool check_save( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    bool                    saved = FALSE;

    if ( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !IS_NPC( ch ) && !IS_NPC( victim ) )
        level /= 2;

    if ( skill->saves )
        switch ( skill->saves ) {
            case SS_POISON_DEATH:
                saved = saves_poison_death( level, victim );
                break;
            case SS_ROD_WANDS:
                saved = saves_wands( level, victim );
                break;
            case SS_PARA_PETRI:
                saved = saves_para_petri( level, victim );
                break;
            case SS_BREATH:
                saved = saves_breath( level, victim );
                break;
            case SS_SPELL_STAFF:
                saved = saves_spell_staff( level, victim );
                break;
        }
    return saved;
}

/*
 * Generic offensive spell damage attack   -Thoric
 * Volk - problems with this spell overriding config nice, PK restrictions etc.
 * look at damage() function and see whether we need checks in this
 */
ch_ret spell_attack( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );
    bool                    saved = check_save( sn, level, ch, victim );
    int                     dam;
    ch_ret                  retcode = rNONE;

    if ( saved && SPELL_SAVE( skill ) == SE_NEGATE ) {
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }
    if ( skill->dice )
        dam = UMAX( 0, dice_parse( ch, level, skill->dice ) );
    else
        dam = dice( 1, level / 2 );
    if ( saved ) {
        switch ( SPELL_SAVE( skill ) ) {
            case SE_3QTRDAM:
                dam = ( dam * 3 ) / 4;
                break;
            case SE_HALFDAM:
                dam >>= 1;
                break;
            case SE_QUARTERDAM:
                dam >>= 2;
                break;
            case SE_EIGHTHDAM:
                dam >>= 3;
                break;

            case SE_ABSORB:                           /* victim absorbs spell for hp's */
                act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, victim, TO_CHAR );
                act( AT_MAGIC, "You absorb $N's $t!", victim, skill->noun_damage, ch, TO_CHAR );
                act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, victim, TO_NOTVICT );
                victim->hit = URANGE( 0, victim->hit + dam, victim->max_hit );
                update_pos( victim );
                if ( ( dam > 0 && ch->fighting && ch->fighting->who == victim )
                     || ( dam > 0 && victim->fighting && victim->fighting->who == ch ) ) {
                    int                     xp =
                        ch->fighting ? ch->fighting->xp : victim->fighting->xp;
                    int                     xp_gain = ( int ) ( xp * dam * 2 ) / victim->max_hit;

                    gain_exp( ch, 0 - xp_gain );
                }

                if ( skill->affects )
                    retcode = spell_affectchar( sn, level, ch, victim );
                return retcode;

            case SE_REFLECT:                          /* reflect the spell to the caster 
                                                        */
                return spell_attack( sn, level, victim, ch );
        }
    }
    retcode = damage( ch, victim, dam, sn );
    if ( retcode == rNONE && skill->affects && !char_died( ch ) && !char_died( victim )
         && ( !is_affected( victim, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE )
              || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
        retcode = spell_affectchar( sn, level, ch, victim );
    return retcode;
}

/*
 * Generic area attack      -Thoric
 */
ch_ret spell_area_attack( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );
    bool                    saved;
    bool                    affects;
    int                     dam;
    bool                    ch_died = FALSE;
    ch_ret                  retcode = rNONE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }

    affects = ( skill->affects ? TRUE : FALSE );
    if ( skill->hit_char && skill->hit_char[0] != '\0' )
        act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
    if ( skill->hit_room && skill->hit_room[0] != '\0' )
        act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );

    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
        vch_next = vch->next_in_room;

        if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
             && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
            continue;

        if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
            continue;

        if ( vch == ch )
            continue;

        if ( is_safe( ch, vch, FALSE ) )
            continue;

        if ( !IS_NPC( ch ) && !IS_NPC( vch ) && !in_arena( ch )
             && ( !IS_PKILL( ch ) || !IS_PKILL( vch ) ) )
            continue;

        if ( skill && SPELL_FLAG( skill, SF_AREA ) && !SPELL_FLAG( skill, SF_GROUPSPELL )
             && is_same_group( vch, ch ) )
            continue;

        saved = check_save( sn, level, ch, vch );
        if ( saved && SPELL_SAVE( skill ) == SE_NEGATE ) {
            failed_casting( skill, ch, vch, NULL );
            continue;
        }
        else if ( skill->dice )
            dam = dice_parse( ch, level, skill->dice );
        else
            dam = dice( 1, level / 2 );
        if ( saved ) {
            switch ( SPELL_SAVE( skill ) ) {
                case SE_3QTRDAM:
                    dam = ( dam * 3 ) / 4;
                    break;
                case SE_HALFDAM:
                    dam >>= 1;
                    break;
                case SE_QUARTERDAM:
                    dam >>= 2;
                    break;
                case SE_EIGHTHDAM:
                    dam >>= 3;
                    break;

                case SE_ABSORB:                       /* victim absorbs spell for hp's */
                    act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, vch, TO_CHAR );
                    act( AT_MAGIC, "You absorb $N's $t!", vch, skill->noun_damage, ch, TO_CHAR );
                    act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, vch, TO_NOTVICT );
                    vch->hit = URANGE( 0, vch->hit + dam, vch->max_hit );
                    update_pos( vch );
                    if ( ( dam > 0 && ch->fighting && ch->fighting->who == vch )
                         || ( dam > 0 && vch->fighting && vch->fighting->who == ch ) ) {
                        int                     xp =
                            ch->fighting ? ch->fighting->xp : vch->fighting->xp;
                        int                     xp_gain = ( int ) ( xp * dam * 2 ) / vch->max_hit;

                        gain_exp( ch, 0 - xp_gain );
                    }
                    continue;

                case SE_REFLECT:                      /* reflect the spell to the caster 
                                                        */
                    retcode = spell_attack( sn, level, vch, ch );
                    if ( char_died( ch ) ) {
                        ch_died = TRUE;
                        break;
                    }
                    continue;
            }
        }
        retcode = damage( ch, vch, dam, sn );
        if ( retcode == rNONE && affects && !char_died( ch ) && !char_died( vch )
             && ( !is_affected( vch, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE )
                  || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
            retcode = spell_affectchar( sn, level, ch, vch );
        if ( retcode == rCHAR_DIED || char_died( ch ) ) {
            ch_died = TRUE;
            break;
        }
    }
    return retcode;
}

/* Volk - here now, stupid blindsight/blindness code */
ch_ret spell_affectchar( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    SMAUG_AFF              *saf;
    SKILLTYPE              *skill = get_skilltype( sn );
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     chance;
    bool                    affected = FALSE,
        first = TRUE;
    ch_ret                  retcode = rNONE;

    if ( SPELL_FLAG( skill, SF_RECASTABLE ) )
        affect_strip( victim, sn );
    for ( saf = skill->affects; saf; saf = saf->next ) {
        if ( saf->location >= REVERSE_APPLY ) {
            if ( !SPELL_FLAG( skill, SF_ACCUMULATIVE ) ) {
                if ( first == TRUE ) {
                    if ( SPELL_FLAG( skill, SF_RECASTABLE ) )
                        affect_strip( ch, sn );
                    if ( is_affected( ch, sn ) )
                        affected = TRUE;
                }
                first = FALSE;
                if ( affected == TRUE )
                    continue;
            }
            victim = ch;
        }
        else
            victim = ( CHAR_DATA * ) vo;
        /*
         * Check if char has this bitvector already 
         */
        af.bitvector = meb( saf->bitvector );
        if ( saf->bitvector >= 0 && xIS_SET( victim->affected_by, saf->bitvector )
             && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
            continue;
        /*
         * necessary for affect_strip to work properly...
         */
        switch ( saf->bitvector ) {
            default:
                af.type = sn;
                break;
            case AFF_POISON:
                af.type = gsn_poison;
                chance = ris_save( victim, level, RIS_POISON );
                if ( chance == 1000 ) {
                    retcode = rVICT_IMMUNE;
                    if ( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                        return retcode;
                    continue;
                }
                if ( saves_poison_death( chance, victim ) ) {
                    if ( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                        return retcode;
                    continue;
                }
                victim->mental_state = URANGE( 30, victim->mental_state + 2, 100 );
                break;
            case AFF_BLINDNESS:
                af.type = gsn_blindness;
                break;
            case AFF_NOSIGHT:
                af.type = gsn_nosight;
                break;
            case AFF_CURSE:
                af.type = gsn_curse;
                break;
            case AFF_INVISIBLE:
                af.type = gsn_invis;
                break;
            case AFF_SILENCE:
                af.type = gsn_silence;
                break;
            case AFF_SLEEP:
                af.type = gsn_sleep;
                chance = ris_save( victim, level, RIS_SLEEP );
                if ( chance == 1000 ) {
                    retcode = rVICT_IMMUNE;
                    if ( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                        return retcode;
                    continue;
                }
                break;
            case AFF_CHARM:
                af.type = gsn_charm_person;
                chance = ris_save( victim, level, RIS_CHARM );
                if ( chance == 1000 ) {
                    retcode = rVICT_IMMUNE;
                    if ( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                        return retcode;
                    continue;
                }
                break;
            case AFF_POSSESS:
                af.type = gsn_possess;
                break;
        }
        af.duration = dice_parse( ch, level, saf->duration );
        if ( af.duration > 0 && IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
            af.duration *= 2;                          /* Incrase the duration by 2 */
        af.modifier = dice_parse( ch, level, saf->modifier );
        af.level = level;
        af.location = saf->location % REVERSE_APPLY;

        if ( af.duration == 0 ) {
            int                     xp_gain;

            switch ( af.location ) {
                case APPLY_HIT:
                    victim->hit = URANGE( 0, victim->hit + af.modifier, victim->max_hit );
                    update_pos( victim );
                    if ( ( af.modifier > 0 && ch->fighting && ch->fighting->who == victim )
                         || ( af.modifier > 0 && victim->fighting
                              && victim->fighting->who == ch ) ) {
                        int                     xp =
                            ch->fighting ? ch->fighting->xp : victim->fighting->xp;

                        xp_gain = ( int ) ( xp * af.modifier * 2 ) / victim->max_hit;
                        gain_exp( ch, 0 - xp_gain );
                    }
                    if ( IS_NPC( victim ) && victim->hit <= 0 )
                        damage( ch, victim, 5, TYPE_UNDEFINED );
                    break;
                case APPLY_MANA:
                    victim->mana = URANGE( 0, victim->mana + af.modifier, victim->max_mana );
                    update_pos( victim );
                    break;
                case APPLY_MOVE:
                    victim->move = URANGE( 0, victim->move + af.modifier, victim->max_move );
                    update_pos( victim );
                    break;
                default:
                    affect_modify( victim, &af, TRUE );
                    break;
            }
        }
        else if ( SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
            affect_join( victim, &af );
        else
            affect_to_char( victim, &af );
    }
    update_pos( victim );
    return retcode;
}

/*
 * Generic spell affect      -Thoric
 * Volk - followed this here chasing blindsight/blindness problem. No sign of get_aflag either.
 */
ch_ret spell_affect( int sn, int level, CHAR_DATA *ch, void *vo )
{
    SMAUG_AFF              *saf;
    SKILLTYPE              *skill = get_skilltype( sn );
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    bool                    groupsp;
    bool                    areasp;
    bool                    hitchar = FALSE,
        hitroom = FALSE,
        hitvict = FALSE;
    ch_ret                  retcode;

// no affect bug just set it for those who may not need affects
    if ( !skill->affects && !skill_lookup( "heal" ) ) {
        bug( "spell_affect has no affects sn %d", sn );
        return rNONE;
    }
    if ( SPELL_FLAG( skill, SF_GROUPSPELL ) )
        groupsp = TRUE;
    else
        groupsp = FALSE;

    if ( SPELL_FLAG( skill, SF_AREA ) )
        areasp = TRUE;
    else
        areasp = FALSE;
    if ( !groupsp && !areasp ) {
        /*
         * Can't find a victim 
         */
        if ( !victim ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        if ( ( IS_IMMUNE( victim, RIS_MAGIC ) ) || IS_IMMUNE( victim, SPELL_DAMAGE( skill ) ) ) {
            immune_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        /*
         * Spell is already on this guy 
         */
        if ( is_affected( victim, sn ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE )
             && !SPELL_FLAG( skill, SF_RECASTABLE ) ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        if ( ( saf = skill->affects ) && !saf->next && saf->location == APPLY_STRIPSN
             && !is_affected( victim, dice_parse( ch, level, saf->modifier ) ) ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        if ( check_save( sn, level, ch, victim ) ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }
    }
    else {
        if ( skill->hit_char && skill->hit_char[0] != '\0' ) {
            if ( strstr( skill->hit_char, "$N" ) )
                hitchar = TRUE;
            else
                act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
        }
        if ( skill->hit_room && skill->hit_room[0] != '\0' ) {
            if ( strstr( skill->hit_room, "$N" ) )
                hitroom = TRUE;
            else
                act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );
        }
        if ( skill->hit_vict && skill->hit_vict[0] != '\0' )
            hitvict = TRUE;
        if ( victim )
            victim = victim->in_room->first_person;
        else
            victim = ch->in_room->first_person;
    }
    if ( !victim ) {
        bug( "spell_affect: could not find victim: sn %d", sn );
        failed_casting( skill, ch, victim, NULL );
        return rSPELL_FAILED;
    }

    for ( ; victim; victim = victim->next_in_room ) {
        if ( groupsp || areasp ) {
            if ( ( groupsp && !is_same_group( victim, ch ) )
                 || ( areasp && !groupsp && is_same_group( victim, ch ) )
                 || IS_IMMUNE( victim, RIS_MAGIC ) || IS_IMMUNE( victim, SPELL_DAMAGE( skill ) )
                 || check_save( sn, level, ch, victim ) || ( !SPELL_FLAG( skill, SF_RECASTABLE )
                                                             && is_affected( victim, sn ) ) )
                continue;

            if ( hitvict && ch != victim ) {
                act( AT_MAGIC, skill->hit_vict, ch, NULL, victim, TO_VICT );
                if ( hitroom ) {
                    act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
                    act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_CHAR );
                }
            }
            else if ( hitroom )
                act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
            if ( ch == victim ) {
                if ( hitvict )
                    act( AT_MAGIC, skill->hit_vict, ch, NULL, ch, TO_CHAR );
                else if ( hitchar )
                    act( AT_MAGIC, skill->hit_char, ch, NULL, ch, TO_CHAR );
            }
            else if ( hitchar )
                act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
        }
        retcode = spell_affectchar( sn, level, ch, victim );
        if ( !groupsp && !areasp ) {
            if ( retcode == rVICT_IMMUNE )
                immune_casting( skill, ch, victim, NULL );
            else
                successful_casting( skill, ch, victim, NULL );
            break;
        }
    }
    return rNONE;
}

/*
 * Generic inventory object spell     -Thoric
 */
ch_ret spell_obj_inv( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( !obj ) {
        failed_casting( skill, ch, NULL, NULL );
        return rNONE;
    }

    switch ( SPELL_ACTION( skill ) ) {
        default:
        case SA_NONE:
            return rNONE;

        case SA_CREATE:
            if ( SPELL_FLAG( skill, SF_WATER ) ) {     /* create water */
                int                     water;
                struct WeatherCell     *cell = getWeatherCell( ch->in_room->area );

                if ( obj->item_type != ITEM_DRINK_CON ) {
                    send_to_char( "It is unable to hold water.\r\n", ch );
                    return rSPELL_FAILED;
                }

                if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 ) {
                    send_to_char( "It contains some other liquid.\r\n", ch );
                    return rSPELL_FAILED;
                }

                water =
                    UMIN( ( skill->dice ? dice_parse( ch, level, skill->dice ) : level ) *
                          ( getPrecip( cell ) >= 0 ? 2 : 1 ), obj->value[0] - obj->value[1] );

                if ( water > 0 ) {
                    separate_obj( obj );
                    obj->value[2] = LIQ_WATER;
                    obj->value[1] += water;
                    if ( !is_name( "water", obj->name ) ) {
                        char                    buf[MSL];

                        snprintf( buf, MSL, "%s water", obj->name );
                        STRFREE( obj->name );
                        obj->name = STRALLOC( buf );
                    }
                }
                successful_casting( skill, ch, NULL, obj );
                return rNONE;
            }
            if ( SPELL_DAMAGE( skill ) == SD_FIRE ) {  /* burn object */
                /*
                 * return rNONE; 
                 */
            }
            if ( SPELL_DAMAGE( skill ) == SD_POISON    /* poison object */
                 || SPELL_CLASS( skill ) == SC_DEATH ) {
                switch ( obj->item_type ) {
                    default:
                        failed_casting( skill, ch, NULL, obj );
                        break;
                    case ITEM_COOK:
                    case ITEM_FOOD:
                    case ITEM_DRINK_CON:
                        separate_obj( obj );
                        obj->value[3] = 1;
                        successful_casting( skill, ch, NULL, obj );
                        break;
                }
                return rNONE;
            }
            if ( SPELL_CLASS( skill ) == SC_LIFE       /* purify food/water */
                 && ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON
                      || obj->item_type == ITEM_COOK ) ) {
                switch ( obj->item_type ) {
                    default:
                        failed_casting( skill, ch, NULL, obj );
                        break;
                    case ITEM_COOK:
                    case ITEM_FOOD:
                    case ITEM_DRINK_CON:
                        separate_obj( obj );
                        obj->value[3] = 0;
                        successful_casting( skill, ch, NULL, obj );
                        break;
                }
                return rNONE;
            }

            if ( SPELL_CLASS( skill ) != SC_NONE ) {
                failed_casting( skill, ch, NULL, obj );
                return rNONE;
            }
            switch ( SPELL_POWER( skill ) ) {          /* clone object */
                    OBJ_DATA               *clone;

                default:
                case SP_NONE:
                    if ( ch->level - obj->level < 10
                         || obj->cost > ch->level * get_curr_int( ch ) * get_curr_wis( ch ) ) {
                        failed_casting( skill, ch, NULL, obj );
                        return rNONE;
                    }
                    break;
                case SP_MINOR:
                    if ( ch->level - obj->level < 20
                         || obj->cost > ch->level * get_curr_int( ch ) / 5 ) {
                        failed_casting( skill, ch, NULL, obj );
                        return rNONE;
                    }
                    break;
                case SP_GREATER:
                    if ( ch->level - obj->level < 5
                         || obj->cost > ch->level * 10 * get_curr_int( ch ) * get_curr_wis( ch ) ) {
                        failed_casting( skill, ch, NULL, obj );
                        return rNONE;
                    }
                    break;
                case SP_MAJOR:
                    if ( ch->level - obj->level < 0
                         || obj->cost > ch->level * 50 * get_curr_int( ch ) * get_curr_wis( ch ) ) {
                        failed_casting( skill, ch, NULL, obj );
                        return rNONE;
                    }
                    clone = clone_object( obj );
                    clone->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
                    obj_to_char( clone, ch );
                    successful_casting( skill, ch, NULL, obj );
                    break;
            }
            return rNONE;

        case SA_DESTROY:
        case SA_RESIST:
        case SA_SUSCEPT:
        case SA_DIVINATE:
            if ( SPELL_DAMAGE( skill ) == SD_POISON ) { /* detect poison */
                if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD
                     || obj->item_type == ITEM_COOK ) {
                    if ( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
                        send_to_char( "It looks undercooked.\r\n", ch );
                    else if ( obj->value[3] != 0 )
                        send_to_char( "You smell poisonous fumes.\r\n", ch );
                    else
                        send_to_char( "It looks very delicious.\r\n", ch );
                }
                else
                    send_to_char( "It doesn't look poisoned.\r\n", ch );
                return rNONE;
            }
            return rNONE;
        case SA_OBSCURE:                              /* make obj invis */
            if ( IS_OBJ_STAT( obj, ITEM_INVIS )
                 || chance( ch, skill->dice ? dice_parse( ch, level, skill->dice ) : 20 ) ) {
                failed_casting( skill, ch, NULL, NULL );
                return rSPELL_FAILED;
            }
            successful_casting( skill, ch, NULL, obj );
            xSET_BIT( obj->extra_flags, ITEM_INVIS );
            return rNONE;

        case SA_CHANGE:
            return rNONE;
    }
    return rNONE;
}

/*
 * Generic object creating spell     -Thoric
 */
ch_ret spell_create_obj( int sn, int level, CHAR_DATA *ch, void *vo )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    int                     lvl;
    int                     vnum = skill->value;
    OBJ_DATA               *obj;
    OBJ_INDEX_DATA         *oi;

    switch ( SPELL_POWER( skill ) ) {
        default:
        case SP_NONE:
            lvl = 10;
            break;
        case SP_MINOR:
            lvl = 0;
            break;
        case SP_GREATER:
            lvl = level / 2;
            break;
        case SP_MAJOR:
            lvl = level;
            break;
    }

    /*
     * Add predetermined objects here
     */
    if ( vnum == 0 ) {
        if ( !str_cmp( target_name, "sword" ) )
            vnum = OBJ_VNUM_SCHOOL_SWORD;
        if ( !str_cmp( target_name, "shield" ) )
            vnum = OBJ_VNUM_SCHOOL_SHIELD;
    }

    if ( ( oi = get_obj_index( vnum ) ) == NULL || ( obj = create_object( oi, lvl ) ) == NULL ) {
        failed_casting( skill, ch, NULL, NULL );
        return rNONE;
    }
    obj->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
    successful_casting( skill, ch, NULL, obj );
    if ( CAN_WEAR( obj, ITEM_TAKE ) )
        obj_to_char( obj, ch );
    else
        obj_to_room( obj, ch->in_room );
    return rNONE;
}

/*
 * Generic mob creating spell     -Thoric
 */
ch_ret spell_create_mob( int sn, int level, CHAR_DATA *ch, void *vo )
{
    SKILLTYPE              *skill = get_skilltype( sn );
    int                     lvl;
    int                     vnum = skill->value;
    CHAR_DATA              *mob;
    MOB_INDEX_DATA         *mi;
    AFFECT_DATA             af;

    /*
     * set maximum mob level 
     */
    switch ( SPELL_POWER( skill ) ) {
        default:
        case SP_NONE:
            lvl = 20;
            break;
        case SP_MINOR:
            lvl = 5;
            break;
        case SP_GREATER:
            lvl = level / 2;
            break;
        case SP_MAJOR:
            lvl = level;
            break;
    }

    /*
     * Add predetermined mobiles here
     */
    if ( vnum == 0 ) {
        if ( !str_cmp( target_name, "cityguard" ) )
            vnum = MOB_VNUM_CITYGUARD;
        if ( !str_cmp( target_name, "vampire" ) )
            vnum = MOB_VNUM_VAMPIRE;
    }

    if ( ( mi = get_mob_index( vnum ) ) == NULL || ( mob = create_mobile( mi ) ) == NULL ) {
        failed_casting( skill, ch, NULL, NULL );
        return rNONE;
    }
    mob->level = UMIN( lvl, skill->dice ? dice_parse( ch, level, skill->dice ) : mob->level );
    mob->armor = interpolate( mob->level, 100, -100 );

    mob->max_hit =
        mob->level * 8 + number_range( mob->level * mob->level / 4, mob->level * mob->level );
    mob->hit = mob->max_hit;
    successful_casting( skill, ch, mob, NULL );
    char_to_room( mob, ch->in_room );
    add_follower( mob, ch );
    af.type = sn;
    af.duration = ( int ) ( ( number_fuzzy( ( level + 1 ) / 3 ) + 1 ) * DUR_CONV );
    af.location = 0;
    af.modifier = 0;
    af.level = ch->level;
    af.bitvector = meb( AFF_CHARM );
    affect_to_char( mob, &af );
    return rNONE;
}

ch_ret                  ranged_attack( CHAR_DATA *, char *, OBJ_DATA *, OBJ_DATA *, short, short );

/*
 * Generic handler for new "SMAUG" spells   -Thoric
 */
ch_ret spell_smaug( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;

    struct skill_type      *skill = get_skilltype( sn );

    /*
     * Put this check in to prevent crashes from this getting a bad skill 
     */

    if ( !skill ) {                                    // FINDME
//    bug("spell_smaug: [SN:]  Called with a null skill for sn %d", sn);
        return rERROR;
    }

    switch ( skill->target ) {
        case TAR_IGNORE:

            /*
             * offensive area spell 
             */
            if ( SPELL_FLAG( skill, SF_AREA )
                 && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                      || ( SPELL_ACTION( skill ) == SA_CREATE
                           && SPELL_CLASS( skill ) == SC_DEATH ) ) )
                return spell_area_attack( sn, level, ch, vo );

            if ( SPELL_ACTION( skill ) == SA_CREATE ) {
                if ( SPELL_FLAG( skill, SF_OBJECT ) )  /* create object */
                    return spell_create_obj( sn, level, ch, vo );
                if ( SPELL_CLASS( skill ) == SC_LIFE ) /* create mob */
                    return spell_create_mob( sn, level, ch, vo );
            }

            /*
             * affect a distant player 
             */
            if ( SPELL_FLAG( skill, SF_DISTANT )
                 && ( victim = get_char_world( ch, target_name ) )
                 && ( !IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
                      || !IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) )
                 && SPELL_FLAG( skill, SF_CHARACTER ) )
                return spell_affect( sn, level, ch, get_char_world( ch, target_name ) );

            /*
             * affect a player in this room (should have been TAR_CHAR_XXX) 
             */
            if ( SPELL_FLAG( skill, SF_CHARACTER ) )
                return spell_affect( sn, level, ch, get_char_room( ch, target_name ) );

            if ( skill->range > 0
                 && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                      || ( SPELL_ACTION( skill ) == SA_CREATE
                           && SPELL_CLASS( skill ) == SC_DEATH ) ) )
                return ranged_attack( ch, ranged_target_name, NULL, NULL, sn, skill->range );
            /*
             * will fail, or be an area/group affect 
             */
            return spell_affect( sn, level, ch, vo );

        case TAR_CHAR_OFFENSIVE:

            // Let's check if attacking a peaceful player - Volk
/*
      if(!IS_NPC(ch) && (victim = get_char_room(ch, target_name)) && !IS_NPC(victim) && (!IS_PKILL(ch) || !IS_PKILL(victim)) )  // one or both PCs are peaceful 
      {
        set_char_color(AT_WHITE, ch);
        send_to_char("You must MURDER a player.\r\n", ch);
        return rNONE;
      }

Volk - works fine but not fixing the problem. Need to follow back to DO_CAST, check target, and if offensive nip this in the bud. 
*/
            /*
             * a regular damage inflicting spell attack 
             */

            if ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                 || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) )
                return spell_attack( sn, level, ch, vo );

            /*
             * a nasty spell affect 
             */
            return spell_affect( sn, level, ch, vo );

        case TAR_CHAR_DEFENSIVE:
        case TAR_CHAR_SELF:
            if ( SPELL_FLAG( skill, SF_NOFIGHT ) &&
                 ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
                   || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
                   || ch->position == POS_BERSERK ) ) {
                send_to_char( "You can't concentrate enough for that!\r\n", ch );
                return rNONE;
            }

            if ( vo && SPELL_ACTION( skill ) == SA_DESTROY ) {

                /*
                 * cure poison 
                 */
                if ( SPELL_DAMAGE( skill ) == SD_POISON ) {
                    if ( is_affected( victim, gsn_poison ) ) {
                        affect_strip( victim, gsn_poison );
                        victim->mental_state = URANGE( -100, victim->mental_state, -10 );
                        successful_casting( skill, ch, victim, NULL );
                        return rNONE;
                    }
                    failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
                /*
                 * cure blindness 
                 */
                if ( SPELL_CLASS( skill ) == SC_ILLUSION ) {
                    if ( is_affected( victim, gsn_blindness ) ) {
                        affect_strip( victim, gsn_blindness );
                        successful_casting( skill, ch, victim, NULL );
                        return rNONE;
                    }
                    failed_casting( skill, ch, victim, NULL );
                    return rSPELL_FAILED;
                }
            }
            return spell_affect( sn, level, ch, vo );

        case TAR_OBJ_INV:
            return spell_obj_inv( sn, level, ch, vo );
    }
    return rNONE;
}

/* Haus' new, new mage spells follow */

/*
 *  4 Energy Spells
 */
ch_ret spell_ethereal_fist( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 35, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 6 ) ) - 31 );
    dam = UMAX( 0, dam );

    if ( saves_spell_staff( level, victim ) )
        dam /= 4;

    return damage( ch, victim, dam, sn );
}

ch_ret spell_spectral_furor( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 16, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 7 ) + 7 ) );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_hand_of_chaos( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 18, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 7 ) + 9 ) );

    if ( saves_spell_staff( level, victim ) )
        dam /= 4;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_disruption( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 14, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 6 ) + 8 ) );

    if ( saves_spell_staff( level, victim ) )
        dam = 0;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_sonic_resonance( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 23, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 8 ) ) );

    if ( saves_spell_staff( level, victim ) )
        dam = dam * 3 / 4;
    return damage( ch, victim, dam, sn );
}

/*
 * 3 Mentalstate spells
 */
ch_ret spell_mind_wrack( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    /*
     * decrement mentalstate by up to 50 
     */

    level = UMAX( 0, level );
    dam = number_range( 0, 0 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_mind_wrench( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    /*
     * increment mentalstate by up to 50 
     */

    level = UMAX( 0, level );
    dam = number_range( 0, 0 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

/* Non-offensive spell! */
ch_ret spell_revive( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    /*
     * set mentalstate to mentalstate/2 
     */
    level = UMAX( 0, level );
    dam = number_range( 0, 0 );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

/*
 * n Acid Spells
 */
ch_ret spell_sulfurous_spray( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 19, level );
    dam = ( int ) ( 1.3 * ( 2 * level * number_range( 1, 7 ) + 11 ) );

    if ( saves_spell_staff( level, victim ) )
        dam /= 4;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_caustic_fount( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 42, level );
    dam = ( int ) ( 1.3 * ( 2 * level * number_range( 1, 7 ) + 11 ) );
    dam = ( int ) ( 1.3 * ( 2 * level * number_range( 1, 6 ) ) - 31 );
    dam = UMAX( 0, dam );

    if ( saves_spell_staff( level, victim ) )
        dam = dam * 1 / 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_acetum_primus( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    dam = ( int ) ( 1.3 * ( 2 * level * number_range( 1, 4 ) + 7 ) );
    if ( saves_spell_staff( level, victim ) )
        dam = 3 * dam / 4;
    return damage( ch, victim, dam, sn );
}

/*
 *  Electrical
 */

ch_ret spell_galvanic_whip( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 10, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 6 ) + 5 ) );
    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_magnetic_thrust( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 29, level );
    dam = ( int ) ( .65 * ( ( 5 * level * number_range( 1, 6 ) ) + 16 ) );
    if ( saves_spell_staff( level, victim ) )
        dam /= 3;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_quantum_spike( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam,
                            l;

    level = UMAX( 0, level );
    l = 2 * ( level / 10 );
    dam = ( int ) ( 1.3 * ( l * number_range( 1, 40 ) + 145 ) );

    if ( saves_spell_staff( level, victim ) )
        dam /= 2;
    return damage( ch, victim, dam, sn );
}

/*
 * Black-magicish guys
 */

/* L2 Mage Spell */
ch_ret spell_black_hand( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 5, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 6 ) + 3 ) );

    if ( saves_spell_staff( level, victim ) )
        dam /= 4;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_black_fist( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;

    level = UMAX( 0, level );
    level = UMIN( 30, level );
    dam = ( int ) ( 1.3 * ( level * number_range( 1, 9 ) + 4 ) );

    if ( saves_spell_staff( level, victim ) )
        dam /= 4;
    return damage( ch, victim, dam, sn );
}

ch_ret spell_black_lightning( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    level = UMAX( 0, level );
    short                   dam;

    if ( ch->secondclass == -1 ) {
        dam = maximum + number_range( 150, 250 );
    }
    else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        dam = maximum;
    }
    else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        dam = maximum - number_range( 100, 150 );
    }
    if ( IS_NPC( ch ) ) {
        dam = maximum + number_range( 150, 250 );
    }
    return damage( ch, victim, dam, sn );
}

ch_ret spell_midas_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = NULL;
    int                     val;
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;

    separate_obj( obj );                               /* nice, alty :) */

    if ( IS_OBJ_STAT( obj, ITEM_NODROP ) ) {
        send_to_char( "You can't seem to let go of it.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "That item is not for mortal hands to touch!\r\n", ch );
        return rSPELL_FAILED;                          /* Thoric */
    }

    if ( GET_VALUE( obj, type ) != CURR_GOLD ) {
        act( AT_MAGIC, "You cannot produce gold from a $p.", ch, obj, NULL, TO_CHAR );
        return rNONE;
    }

    if ( !CAN_WEAR( obj, ITEM_TAKE ) || ( obj->item_type == ITEM_CORPSE_NPC )
         || ( obj->item_type == ITEM_CORPSE_PC ) ) {
        send_to_char( "You cannot seem to turn this item to gold!", ch );
        return rNONE;
    }

    val = ( obj->cost / 10 ) + 1;
    val = UMAX( 0, val );

    // Reworked val, and Capped.. -Taon
    if ( val > 30 )
        val = 30;

    GET_MONEY( ch, DEFAULT_CURR ) += val;

    if ( obj_extracted( obj ) )
        return rNONE;
    extract_obj( obj );
    act( AT_YELLOW, "You transmogrify $p into gold!", ch, obj, NULL, TO_CHAR );
    act( AT_YELLOW, "$n points at $p, causing it to glow and take the shape of gold coins!", ch,
         obj, NULL, TO_ROOM );
    return rNONE;
}

ch_ret spell_remove_silence( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL ) {
        return rSPELL_FAILED;
    }

    if ( !xIS_SET( victim->act, PLR_SILENCE ) ) {
        send_to_char( "Your intended victim is not silenced.\r\n", ch );
        return rNONE;
    }
    else {
        affect_strip( victim, gsn_silence );
        REMOVE_AFFECTED( victim, AFF_SILENCE );
        xREMOVE_BIT( victim->act, PLR_SILENCE );
        act( AT_MAGIC, "$n points at you and utters the word, 'vocalas purus'!", ch, NULL, victim,
             TO_VICT );
        act( AT_MAGIC, "You point at $N and utter the word, vocalas purus'!", ch, NULL, victim,
             TO_CHAR );
        send_to_char( "Your throat suddenly cools, and you gain your vocal ability!\r\n", victim );
        return rNONE;
    }
    failed_casting( skill, ch, victim, NULL );
    return rSPELL_FAILED;
}

ch_ret spell_silence( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    int                     tmp;
    SKILLTYPE              *skill = get_skilltype( sn );

    tmp = level;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL )
        return rSPELL_FAILED;

    if ( ch == victim ) {
        send_to_char( "You cannot silence yourself.\r\n", ch );
        return rNONE;
    }

    if ( xIS_SET( victim->act, PLR_SILENCE ) ) {
        send_to_char( "Your intended victim is already silenced.\r\n", ch );
        return rNONE;
    }
    else {
        if ( IS_AFFECTED( victim, AFF_SILENCE ) || saves_spell_staff( tmp, victim ) ) {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
        }

        af.type = sn;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.duration = level;
        af.bitvector = meb( AFF_SILENCE );
        af.level = level;
        affect_to_char( victim, &af );

        xSET_BIT( victim->act, PLR_SILENCE );
        act( AT_MAGIC, "$n points at you and utters the word, 'vocalas nomas'!", ch, NULL, victim,
             TO_VICT );
        act( AT_MAGIC, "You point at $N and utter the word, vocalas nomas'!", ch, NULL, victim,
             TO_CHAR );
        send_to_char( "Your throat suddenly burns, and you lose your vocal ability!\r\n", victim );
        return rNONE;
    }
    failed_casting( skill, ch, victim, NULL );
    return rSPELL_FAILED;
}

ch_ret spell_fascinate( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    short                   chance;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    CHAR_DATA              *rch;

    if ( ( victim = get_char_room( ch, target_name ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "This magic only works on dumb beasts.\r\n", ch );
        return rSPELL_FAILED;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return rSPELL_FAILED;

    if ( IS_AFFECTED( victim, AFF_FASCINATE ) ) {
        send_to_char( "They are already fascinated.\r\n", ch );
        return rNONE;
    }

    if ( victim->level > ch->level ) {
        chance = number_chance( 1, 4 );
        if ( chance > 2 ) {
            af.type = sn;
            af.duration = 4;
            af.level = ch->level;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = meb( AFF_FASCINATE );
            affect_join( victim, &af );
            act( AT_CYAN,
                 "You draw the outline of a symbol in the air.\r\n&YThe symbol glows a fascinating bright yellow!",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_YELLOW, "$n creates a fascinating bright symbol in the air at $N!", ch, NULL,
                 victim, TO_ROOM );
            for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
                if ( is_same_group( ch, rch ) )
                    stop_fighting( rch, TRUE );
            }
            stop_fighting( victim, TRUE );
            stop_hunting( victim );
            stop_hating( victim );
            ch->hate_level = 0;
            return rNONE;
        }
        else
            return rSPELL_FAILED;
    }

    if ( victim->level <= ch->level ) {
        af.type = sn;
        af.duration = 5;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_FASCINATE );
        affect_join( victim, &af );
        act( AT_CYAN,
             "You draw the outline of a symbol in the air.\r\n&YThe symbol glows a fascinating bright yellow!",
             ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "$n creates a fascinating bright symbol in the air at $N!", ch, NULL,
             victim, TO_ROOM );
        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
            if ( is_same_group( ch, rch ) )
                stop_fighting( rch, TRUE );
        }
        stop_fighting( victim, TRUE );
        stop_hunting( victim );
        stop_hating( victim );
        ch->hate_level = 0;
        return rNONE;
    }

    return rNONE;
}

ch_ret spell_bestow_vitae( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( !VLD_STR( target_name ) )
        victim = ch;
    else if ( ( victim = get_char_room( ch, target_name ) ) == NULL ) {
        send_to_char( "They're not here.\r\n", ch );
        return rNONE;
    }

    // Prevent casting this on someone you're fighting. -Taon
    if ( ch->fighting && who_fighting( ch ) == victim ) {
        send_to_char( "You cannot bestow vitae to someone you're fighting.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( victim, AFF_REACTIVE ) ) {
        send_to_char( "It is already bestowed.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = ch->level + 40;
    else
        af.duration = ch->level + 80;

    af.bitvector = meb( AFF_REACTIVE );
    affect_to_char( victim, &af );

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/bestowvitae.wav)\r\n", ch );

    act( AT_MAGIC, "$n says, 'Besto'vala Vi'taae' and gestures dramatically.", ch, NULL, NULL,
         TO_ROOM );
    act( AT_MAGIC, "You say, 'Besto'vala Vi'taae' and gesture dramatically.", ch, NULL, NULL,
         TO_CHAR );

    return rNONE;
}

//spellcaster raises strength with this spell. -Taon 
//Status: nearly completed, in testing phases.
ch_ret spell_giant_strength( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( target_name[0] == '\0' )
        victim = ch;
    else if ( ( victim = get_char_room( ch, target_name ) ) == NULL ) {
        send_to_char( "They're not here.\r\n", ch );
        return rNONE;
    }

    // Prevent casting this on someone you're fighting. -Taon
    if ( ch->fighting && who_fighting( ch ) == victim ) {
        send_to_char( "You cannot giant strength someone you're fighting.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( victim, AFF_GIANT_STRENGTH ) ) {
        send_to_char( "They already have the strength of a giant.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_STR;

    if ( ch->level < 20 )
        af.modifier = 1;
    else if ( ch->level < 40 )
        af.modifier = number_chance( 1, 2 );
    else if ( ch->level < 60 )
        af.modifier = number_chance( 2, 3 );
    else if ( ch->level < 80 )
        af.modifier = number_chance( 3, 4 );
    else
        af.modifier = 4;

    af.level = ch->level;

    af.duration = ch->level * ( ch->level / 12 );

    af.bitvector = meb( AFF_GIANT_STRENGTH );
    affect_to_char( victim, &af );

    if ( victim != ch )
        act( AT_MAGIC, "Your muscles ripple with the strength of a giant.", ch, NULL, victim,
             TO_VICT );
    else
        act( AT_MAGIC, "Your muscles ripple with the strength of a giant.", ch, NULL, NULL,
             TO_CHAR );

    return rNONE;
}

//Fairly basic spell suprised it wasn't made yet. Completed: 8/11/07, By: Taon
ch_ret spell_slow( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    victim = get_char_room( ch, target_name );

    if ( target_name[0] == '\0' )
        victim = who_fighting( ch );

    if ( victim == ch ) {
        send_to_char( "You can't use slow on yourself.\r\n", ch );
        return rNONE;
    }
    /*
     * if(IS_AFFECTED(victim, AFF_SLOW)) { send_to_char("They're already affected by
     * slowness.\r\n", ch); return rNONE; } 
     */
    if ( IS_IMMORTAL( victim ) && number_chance( 1, 10 ) >= 8 ) {
        send_to_char( "A magical force bounces the spell at you.", ch );
        victim = ch;
    }

    if ( number_chance( 1, 10 ) >= 4 ) {
        af.bitvector = meb( AFF_SLOW );
        af.type = sn;
        af.duration = ( int ) ( ( ch->level - get_curr_int( victim ) ) * 2 );
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        affect_to_char( victim, &af );

        if ( victim != ch )
            ch_printf( ch, "\r\n&GYou notice %s slowing down their pace.\r\n", victim->name );

        send_to_char( "\r\n&GYou feel yourself slowing down.\r\n", victim );

    }
    else
        send_to_char( "\r\n&cYou've failed to slow down your target!\r\n", ch );

    return rNONE;
}

ch_ret spell_nettle_skin( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( target_name[0] == '\0' )
        victim = ch;
    else
        victim = get_char_room( ch, target_name );

    if ( IS_AFFECTED( victim, AFF_NETTLE ) ) {
        send_to_char( "They already have nettle skin.\r\n", ch );
        return rNONE;
    }

    af.bitvector = meb( AFF_NETTLE );
    af.type = sn;

    af.duration = ch->level * 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.level = ch->level;
    affect_to_char( victim, &af );

    if ( victim == ch ) {
        act( AT_ORANGE, "\r\nLarge barbed nettles begin to grow from your skin.", ch, NULL, NULL,
             TO_CHAR );
    }
    else {
        act( AT_ORANGE, "\r\nLarge barbed nettles begin to grow from $N's skin.", ch, NULL, victim,
             TO_CHAR );
    }
    act( AT_ORANGE, "\r\nLarge barbed nettles begin to grow from $n's skin.", ch, NULL, NULL,
         TO_ROOM );
    act( AT_ORANGE, "\r\nLarge barbed nettles begin to grow from your skin.", ch, NULL, victim,
         TO_VICT );
    return rNONE;
}

ch_ret spell_barbs( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    short                   dam;

    dam = 10;
    damage( ch, victim, dam, sn );
    return rNONE;
}

ch_ret spell_greater_smite( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    short                   dam;

    if ( ch->secondclass == -1 ) {
        dam = insane - number_range( 150, 200 );
    }
    if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        dam = insane - number_range( 250, 400 );
    }
    if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        dam = insane - number_range( 450, 600 );
    }
    act( AT_YELLOW, "You summon the greater smite, and lightning slams into $N!", ch, NULL, victim,
         TO_CHAR );
    act( AT_YELLOW, "$n summon's the greater smite, and lightning slams into you!", ch, NULL,
         victim, TO_VICT );
    act( AT_YELLOW, "$n summon's the greater smite, and lightning slams into $N!", ch, NULL, victim,
         TO_NOTVICT );
    damage( ch, victim, dam, sn );
    return rNONE;
}

ch_ret spell_chain_lightning( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch_next,
                           *vch,
                           *victim = ( CHAR_DATA * ) vo;
    bool                    ch_died = FALSE;
    ch_ret                  retcode = rNONE;
    int                     dam = dice( level, 6 );

    if ( victim )
        damage( ch, victim, dam, sn );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( !vch->in_room || victim == vch )
            continue;

        if ( vch->in_room == ch->in_room ) {

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            if ( !IS_NPC( vch ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;

            dam = dice( UMAX( 1, level - 1 ), 6 );

            if ( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) )
                retcode = damage( ch, vch, dam, sn );

            if ( retcode == rCHAR_DIED || char_died( ch ) ) {
                ch_died = TRUE;
                continue;
            }

            if ( char_died( vch ) )
                continue;
        }

        if ( !ch_died && vch->in_room->area == ch->in_room->area && vch->in_room != ch->in_room ) {
            set_char_color( AT_MAGIC, vch );
            send_to_char( "You hear a loud thunderclap...\r\n", vch );
        }
    }

    if ( ch_died )
        return rCHAR_DIED;
    else
        return rNONE;
}

ch_ret spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA            *paf;
    int                     chance;

    if ( obj->item_type != ITEM_ARMOR || IS_OBJ_STAT( obj, ITEM_MAGIC ) || obj->first_affect ) {
        act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL,
             TO_CHAR );
        act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL,
             TO_NOTVICT );
        return rSPELL_FAILED;
    }

    if ( obj->level > ch->level ) {
        act( AT_MAGIC,
             "Your magic twists and winds around $p but is not strong enough for this object.", ch,
             obj, NULL, TO_CHAR );
        return rSPELL_FAILED;
    }

    obj->level = ch->level;                            // changed so you cant enchant
    // armor for newbs making super
    // newbs
    /*
     * Bug fix here. -- Alty 
     */

    chance = number_range( 1, 4 );

    if ( chance == 1 ) {
        separate_obj( obj );
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = APPLY_MOVE;
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            paf->modifier = ch->level / 4;             /* Bug fix and formula change
                                                        * here. Taon */
        }
        else {
            paf->modifier = ch->level / 5;             /* Bug fix and formula change
                                                        * here. Taon */
        }
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
    }
    else if ( chance == 2 ) {
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = APPLY_HIT;
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            paf->modifier = ch->level / 4;             /* Bug fix and formula change
                                                        * here. Taon */
        }
        else {
            paf->modifier = ch->level / 5;             /* Bug fix and formula change
                                                        * here. Taon */
        }

        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
    }
    else if ( chance == 3 ) {
        // Added a little mana to it as-well. Taon
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = APPLY_MANA;
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            paf->modifier = ch->level / 4;             /* Bug fix and formula change
                                                        * here. Taon */
        }
        else {
            paf->modifier = ch->level / 5;             /* Bug fix and formula change
                                                        * here. Taon */
        }

        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
    }
    else if ( chance == 4 ) {
        act( AT_MAGIC, "You lost your concentration while binding $p.", ch, obj, NULL, TO_CHAR );
        act( AT_MAGIC, "$n failes to bind $p properly.", ch, obj, NULL, TO_NOTVICT );
        return rSPELL_FAILED;
    }
    if ( IS_GOOD( ch ) ) {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
        act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_ROOM );
        act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_CHAR );
    }
    else if ( IS_EVIL( ch ) ) {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
        act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_CHAR );
        act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_ROOM );
    }
    else {
        xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
        xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
        act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_ROOM );
        act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_CHAR );
    }
    xSET_BIT( obj->extra_flags, ITEM_MAGIC );
    return rNONE;
}

ch_ret spell_wizard_eye( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *mob;
    MOB_INDEX_DATA         *pMobIndex;
    AFFECT_DATA             af;
    char                    buf[MSL];

    pMobIndex = 0;

    if ( IS_AFFECTED( ch, AFF_WIZARD_EYE ) ) {
        send_to_char( "You can only concentrate enough to control one wizard eye.\r\n", ch );
        return rNONE;
    }

    if ( get_mob_index( MOB_VNUM_WIZARD_EYE ) == NULL ) {
        bug( "Spell_wizard_eye: EYE vnum %d doesn't exist.", MOB_VNUM_WIZARD_EYE );
        return rSPELL_FAILED;
    }
    ch->mana = ch->mana - 10;
    mob = create_mobile( get_mob_index( MOB_VNUM_WIZARD_EYE ) );
    snprintf( buf, MSL, "%s's wizard eye", ch->name );
    STRFREE( mob->short_descr );
    mob->short_descr = STRALLOC( buf );

    mob->level = ch->level / 2;

    mob->max_hit = ch->max_hit / 10;

    mob->hit = mob->max_hit;
    mob->damroll = ch->level / 8;
    mob->hitroll = ch->level / 6;
    mob->alignment = ch->alignment;
    af.type = sn;
    af.duration = 40 + ch->level / 10;
    af.location = 0;
    af.modifier = 0;
    af.level = ch->level;
    af.bitvector = meb( AFF_WIZARD_EYE );
    affect_to_char( mob, &af );
    af.type = sn;
    af.duration = 40 + ch->level / 10;
    af.location = 0;
    af.level = ch->level;
    af.modifier = 0;
    af.bitvector = meb( AFF_WIZARD_EYE );
    affect_to_char( ch, &af );
    char_to_room( mob, ch->in_room );
    act( AT_MAGIC, "$n creates a floating wizard eye in a flash of light!", ch, NULL, NULL,
         TO_ROOM );
    act( AT_MAGIC, "You create a floating wizard eye in a flash of light!", ch, NULL, NULL,
         TO_CHAR );

    return rNONE;

}

ch_ret spell_whirlwind( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam;
    int                     chance;

    AFFECT_DATA             af;

    chance = number_chance( 1, 100 );

    send_to_char( "You call forth the power of the wind!\r\n", ch );
    act( AT_MAGIC, "$n calls forth the power of the wind!", ch, NULL, victim, TO_ROOM );
    act( AT_MAGIC, "The wind guided by your thoughts rushes forth at $N!", ch, NULL, victim,
         TO_CHAR );
    dam = dice( level / 2, 8 );
    damage( ch, victim, dam, sn );

    if ( IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_HITROLL;
    af.modifier = -2;
    af.duration = 10;
    af.bitvector = meb( AFF_BLINDNESS );
    af.level = ch->level;
    affect_to_char( victim, &af );

    if ( chance > 25 ) {
        act( AT_WHITE, "Slammed down with the full force of the wind $N blacks out!", ch, NULL,
             victim, TO_CHAR );
        act( AT_WHITE, "Slammed down by the wind you black out!", ch, NULL, victim, TO_VICT );
        set_position( victim, POS_STUNNED );
        return rNONE;
    }
    return rNONE;
}

/* Volk : looks like this could use a fine tune. Fixed. */
ch_ret spell_mana_pool( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *rch;
    AFFECT_DATA             af;

    act( AT_MAGIC, "$n says, 'Tulsoo lok nardan', and gestures in a broad circle.", ch, NULL, NULL,
         TO_ROOM );
    send_to_char( "&BYou say, 'Tulsoo lok nardan', and gesture in a broad circle.\r\n", ch );

    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
        if ( is_same_group( ch, rch ) ) {
            if ( IS_AFFECTED( rch, AFF_MANA_POOL ) )
                continue;

            if ( ch == rch )
                act( AT_MAGIC,
                     "You invoke a field of energy around yourself that enhances mana regeneration.",
                     ch, NULL, NULL, TO_CHAR );
            else
                act( AT_MAGIC, "$n invokes a mana restoring field of energy around you.", rch, NULL,
                     NULL, TO_CHAR );

            af.bitvector = meb( AFF_MANA_POOL );
            af.type = sn;
            af.duration = ch->level + 50;
            af.location = APPLY_NONE;
            af.level = ch->level;
            af.modifier = 0;
            affect_to_char( rch, &af );

        }
    }
    return rNONE;
}

ch_ret spell_shadowform( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *rch;
    AFFECT_DATA             af;

    act( AT_MAGIC, "$n says, 'Julsoa lok Vardan', and gestures in a broad circle.", ch, NULL, NULL,
         TO_ROOM );
    send_to_char( "&BYou say, 'Julsoa lok Vardan', and gesture in a broad circle.\r\n", ch );

    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
        if ( is_same_group( ch, rch ) ) {
            if ( ch == rch )
                act( AT_MAGIC, "Your body takes on a shadowy form in appearance.", ch, NULL, NULL,
                     TO_CHAR );
            else
                act( AT_MAGIC, "$n causes a shadowy field to surround everyone.", rch, NULL, NULL,
                     TO_CHAR );

            affect_strip( rch, skill_lookup( "shadowform" ) );

            if ( !IS_AFFECTED( rch, AFF_PASS_DOOR ) ) {
                af.bitvector = meb( AFF_PASS_DOOR );
                af.type = sn;
                if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
                    af.duration = ch->level * 2;
                else
                    af.duration = ch->level + 20;
                af.location = APPLY_AFFECT;
                af.level = ch->level;
                af.modifier = 0;
                affect_to_char( rch, &af );
            }

            xCLEAR_BITS( af.bitvector );
            af.type = sn;
            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
                af.duration = ch->level * 2;
            else
                af.duration = ch->level + 20;
            af.location = APPLY_RESISTANT;
            af.level = ch->level;
            af.modifier = 8192;
            affect_to_char( rch, &af );

            xCLEAR_BITS( af.bitvector );
            af.type = sn;
            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
                af.duration = ch->level * 2;
            else
                af.duration = ch->level + 20;
            af.location = APPLY_RESISTANT;
            af.level = ch->level;
            af.modifier = 1;
            affect_to_char( rch, &af );

            xCLEAR_BITS( af.bitvector );
            af.type = sn;
            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) )
                af.duration = ch->level * 2;
            else
                af.duration = ch->level + 20;
            af.location = APPLY_RESISTANT;
            af.level = ch->level;
            af.modifier = 2;
            affect_to_char( rch, &af );
        }
    }
    return rNONE;
}

ch_ret spell_wizard_sight( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_WIZARD_SIGHT ) ) {
        send_to_char( "&BYou are already affected by this spell.&w\r\n", ch );
        return rNONE;
    }

    af.bitvector = meb( AFF_WIZARD_SIGHT );
    af.type = sn;
    af.duration = ch->level * 3 + 25;
    af.location = APPLY_NONE;
    af.level = ch->level;
    af.modifier = 0;
    affect_to_char( ch, &af );
    act( AT_MAGIC, "$n gestures dramatically, then points to his eyes and speaks a word of power.",
         ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You touch your eyes and speak a word of power.", ch, NULL, NULL, TO_CHAR );
    send_to_char( "&BYou see a bird's eye view of your position.&w\r\n", ch );
    return rNONE;

}

ch_ret spell_battlefield_view( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_BATTLEFIELD ) ) {
        send_to_char( "&BYou are already affected by this spell.&w\r\n", ch );
        return rNONE;
    }

    af.bitvector = meb( AFF_BATTLEFIELD );
    af.type = sn;
    af.duration = ch->level * 2;
    af.location = APPLY_NONE;
    af.level = ch->level;
    af.modifier = 0;
    affect_to_char( ch, &af );
    act( AT_MAGIC,
         "$n gestures dramatically, then points to his eyes and speaks a word of greater power.",
         ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You touch your eyes and speak a word of greater power.", ch, NULL, NULL,
         TO_CHAR );
    send_to_char( "&BYou get a mental image of the entire battlefield around you.&w\r\n", ch );
    return rNONE;

}

ch_ret spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( IS_AFFECTED( victim, AFF_GRENDALS_STANCE ) ) {
        send_to_char( "&BMagic cannot aid one utilizing the grendals stance.&w\r\n", ch );
        return rNONE;
    }

    if ( victim->move >= victim->max_move ) {
        send_to_char( "They do not need your revitalizing energies.\r\n", ch );
        return rNONE;
    }

    act( AT_MAGIC, "You focus on calling forth revitalizing energies.", ch, NULL, NULL, TO_CHAR );
    act( AT_WHITE, "You suddenly feel alive with revitalizing energies!", ch, NULL, victim,
         TO_VICT );

    if ( victim->move < victim->max_move ) {
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            victim->move =
                URANGE( victim->move, victim->move + ( ch->level + get_curr_int( ch ) * 20 ),
                        victim->max_move );
        }
        else {
            victim->move =
                URANGE( victim->move, victim->move + ( ch->level + get_curr_int( ch ) * 5 ),
                        victim->max_move );
        }
    }

    if ( !IS_NPC( victim ) && ch->level > 20 ) {
        victim->pcdata->condition[COND_THIRST] = 30;
        victim->pcdata->condition[COND_FULL] = 30;
    }
    return rNONE;
}

/*  Allows priests to gain experience from healing 
 * Based on how much they heal, as well as their level and level of the victim */
void heal_exp( CHAR_DATA *ch, CHAR_DATA *victim, int heal )
{
    int                     gain;

    if ( IS_NPC( ch ) )                                // Forget mobs.. -Taon
        return;

    if ( ( ch->level + victim->level ) > ( ch->level * 3 ) )
        gain = ch->level * 3;
    else
        gain = ( ch->level + victim->level ) * heal;

    gain_exp( ch, gain );
    return;
}

ch_ret spell_arch_healing( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Lets stop the healing of mobs whom the character is fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You can't bring yourself to heal your foe!\r\n", ch );
        return rNONE;
    }

    if ( victim->hit < victim->max_hit ) {
        act( AT_MAGIC, "$N's begins to heal as you focus on their wounds.", ch, NULL, victim,
             TO_NOTVICT );
        if ( victim != ch ) {
            act( AT_MAGIC, "You focus on $N's wounds, giving them a warmth of healing.", ch, NULL,
                 victim, TO_CHAR );
            act( AT_MAGIC, "Your wounds begin to heal and you feel much better.", ch, NULL, victim,
                 TO_VICT );
        }
        else
            act( AT_MAGIC, "Your wounds start to heal, leaving you feeling better.", ch, NULL,
                 victim, TO_CHAR );

        if ( ch->faith < 25 )
            victim->hit += 40 + ( ch->level );
        else
            victim->hit += 70 + ( ch->level );

        if ( ch->level > 50 )
            victim->hit += get_curr_wis( ch );

        if ( victim->hit > victim->max_hit ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You are at full health.\r\n", victim );
            victim->hit = victim->max_hit;
        }
        return rNONE;
    }
    else {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "They're already at maximum health.\r\n", ch );
        return rNONE;
    }
}

ch_ret spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Lets stop the healing of mobs whom the character is fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You can't bring yourself to heal your foe!\r\n", ch );
        return rNONE;
    }
    if ( victim->hit < victim->max_hit ) {
        act( AT_MAGIC, "$N's critical wounds heal and $E seems much better.", ch, NULL, victim,
             TO_NOTVICT );
        if ( victim != ch ) {
            act( AT_MAGIC, "You focus on $N's critical wounds, healing them and restoring $M.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_MAGIC, "Your critical wounds heal and you feel much better.", ch, NULL, victim,
                 TO_VICT );
        }
        else
            act( AT_MAGIC, "Your critical wounds heal and you feel much better.", ch, NULL, victim,
                 TO_CHAR );
        victim->hit += 70 + ( ch->level );
        if ( victim->hit > victim->max_hit ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You are at full health.\r\n", victim );
            victim->hit = victim->max_hit;
        }
        return rNONE;
    }
    else {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "They're already at maximum health.\r\n", ch );
        return rNONE;
    }
}

ch_ret spell_heal( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Stop the healing of mobs whom someone is fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You cant bring yourself to heal your foe.\r\n", ch );
        return rNONE;
    }
    if ( victim->hit < victim->max_hit ) {
        act( AT_MAGIC, "$n's healing touch washes over $N's wounds.", ch, NULL, victim,
             TO_NOTVICT );
        if ( victim != ch ) {
            act( AT_MAGIC, "You focus on $N's wounds, and your healing touch washes over $M.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_MAGIC, "Your wounds fade as $n's healing touch washes over you.", ch, NULL,
                 victim, TO_VICT );
        }
        else
            act( AT_MAGIC, "Your healing touch washes over you, mending your wounds.", ch, NULL,
                 victim, TO_CHAR );
        victim->hit += 100 + ( ch->level );
        if ( victim->hit > victim->max_hit ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You are at full health.\r\n", victim );
            victim->hit = victim->max_hit;
        }
        learn_from_success( ch, gsn_heal );
    }
    else {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "They're already at maximum health.\r\n", ch );
    }
    return rNONE;
}

ch_ret spell_greater_heal( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Prevent ch from healing a mob they're fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You cant bring yourself to heal your foe.\r\n", ch );
        return rNONE;
    }

    if ( victim->hit < victim->max_hit ) {
        if ( victim != ch ) {
            act( AT_MAGIC,
                 "&BYou utter the words, 'greater heal'.\r\n&cTheir wounds begin to close and heal up.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "\r\n&cYour wounds begin to close and heal up.", ch, NULL, victim,
                 TO_VICT );
        }
        else {
            act( AT_MAGIC, "\r\n&c$n's wounds begin to close up and heal.", victim, NULL, NULL,
                 TO_ROOM );
            act( AT_MAGIC, "\r\n&cYour wounds begin to close up and heal up.", ch, NULL, victim,
                 TO_CHAR );
        }
        victim->hit += 350 + ( ch->level );
        if ( victim->hit > victim->max_hit ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You are at full health.\r\n", victim );
            victim->hit = victim->max_hit;
        }
        return rNONE;
    }
    else {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "They're already at maximum health.\r\n", ch );
        return rSPELL_FAILED;
    }
}

ch_ret spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Prevent ch from healing a mob they're fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You cant bring yourself to heal your foe.\r\n", ch );
        return rNONE;
    }
    if ( victim->hit < victim->max_hit ) {
        act( AT_MAGIC, "You feel your wounds close and begin to heal.", ch, NULL, victim, TO_VICT );
        act( AT_MAGIC, "$N's wounds close, and begin to heal.", ch, NULL, victim, TO_NOTVICT );
        act( AT_MAGIC, "You focus on closing $N's wounds and cure some of their health.", ch, NULL,
             victim, TO_CHAR );
        victim->hit += 30 + ( ch->level );
        if ( victim->hit > victim->max_hit ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You are at full health.\r\n", victim );
            victim->hit = victim->max_hit;
        }
        return rNONE;
    }
    else {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "They're already at maximum health.\r\n", ch );
        return rSPELL_FAILED;
    }
}

ch_ret spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    // Prevent ch from healing mob they're fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You cant bring yourself to heal your foe.\r\n", ch );
        return rNONE;
    }
    if ( victim->hit < victim->max_hit ) {
        act( AT_MAGIC, "You feel a little better.", ch, NULL, victim, TO_VICT );
        act( AT_MAGIC, "$N looks a little better.", ch, NULL, victim, TO_NOTVICT );
        act( AT_MAGIC, "$N's wounds suddenly begin to start healing.", ch, NULL, victim, TO_CHAR );
        if ( victim->level < 20 ) {
            victim->hit += number_range( 1, 6 );
        }
        if ( victim->level > 19 && victim->level < 60 ) {
            victim->hit += number_range( 3, 12 );
        }
        if ( victim->level > 59 ) {
            victim->hit += number_range( 5, 20 );
        }

        if ( victim->hit >= victim->max_hit ) {
            victim->hit = victim->max_hit;
        }
        return rNONE;
    }
    else {
        return rSPELL_FAILED;
    }
}

OBJ_DATA               *find_holywater( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;

    for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
        if ( can_see_obj( ch, obj ) ) {
            if ( obj->item_type == ITEM_DRINK_CON && obj->value[1] > 0 && obj->value[2] == 20 )
                return obj;
        }
    }
    return NULL;
}

ch_ret spell_bless_water( int sn, int level, CHAR_DATA *ch, void *vo )
{
    OBJ_DATA               *water = ( OBJ_DATA * ) vo;

    set_char_color( AT_MAGIC, ch );

    if ( water->item_type == ITEM_DRINK_CON && water->value[1] > 0 && water->value[2] == 0 ) {
        water->value[2] = 20;
        act( AT_MAGIC, "$p glows brightly as $n blesses it.", ch, water, NULL, TO_ROOM );
        act( AT_MAGIC, "$p glows brightly as you bless it.", ch, water, NULL, TO_CHAR );
        save_char_obj( ch );
        return rNONE;
    }
    else
        send_to_char( "That's not a container of plain water.\r\n", ch );

    return rSPELL_FAILED;
}

void do_anoint( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *hw;
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        send_to_char( "They are not here.\r\n", ch );
        return;
    }

    if ( ( hw = find_holywater( ch ) ) == NULL ) {
        send_to_char( "You must have holy water to anoint.\r\n", ch );
        return;
    }

    int                     chance = number_percent(  );

    if ( ch != victim ) {
        if ( IS_AFFECTED( victim, AFF_ANOINT ) ) {
            send_to_char( "They have already been anointed.\r\n", ch );
            return;
        }
        if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_anoint] < chance ) {
            ch_printf( ch, "You bath %s's head with holy water, but forget the right prayers.\r\n",
                       victim->name );
            learn_from_failure( ch, gsn_anoint );
            return;
        }
        act( AT_MAGIC, "$n bathes your head in holy water while reciting a prayer to $s god.", ch,
             NULL, victim, TO_VICT );
        act( AT_MAGIC, "You bathe $N's head in holy water while reciting a prayer to your god.", ch,
             NULL, victim, TO_CHAR );
        act( AT_MAGIC, "$n bathes $N's head in holy water while reciting a prayer to $s god.", ch,
             NULL, victim, TO_NOTVICT );
    }
    else {
        if ( IS_AFFECTED( ch, AFF_ANOINT ) ) {
            send_to_char( "You are already anointed.\r\n", ch );
            return;
        }
        if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_anoint] < chance ) {
            send_to_char( "You pour holy water over yourself, but forget the right prayers.\r\n",
                          ch );
            learn_from_failure( ch, gsn_anoint );
            return;
        }

        if ( ch->pcdata->deity ) {
            ch_printf( ch, "&BYou bathe yourself in holy water while reciting a prayer to %s.",
                       ch->pcdata->deity->name );

        }
        else {
            act( AT_MAGIC,
                 "&BYou bathe yourself in holy water while reciting a prayer to your gods.", ch,
                 NULL, NULL, TO_CHAR );
        }
        act( AT_MAGIC, "&B$n bathes $mself in holy water while reciting a prayer to $s gods.", ch,
             NULL, NULL, TO_ROOM );

    }
    af.type = gsn_anoint;
    af.duration = number_range( ch->level * 3, ch->level * 5 );
    af.location = APPLY_STR;
    af.modifier = 2;
    af.bitvector = meb( AFF_ANOINT );
    af.level = ch->level;
    affect_to_char( victim, &af );
    hw->value[1]--;
    learn_from_success( ch, gsn_anoint );
    return;
}
