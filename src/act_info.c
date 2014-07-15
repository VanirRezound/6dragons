/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Informational module                                                  *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <time.h>
#if !defined(WIN32)
#include <crypt.h>
#endif
#include <sys/stat.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/pfiles.h"
#include "h/polymorph.h"
#include "h/hint.h"
#include "h/ftag.h"
#include "h/city.h"

void                    get_curr_players( void );
extern int              num_players_online;
bool check_social       args( ( CHAR_DATA *ch, char *command, char *argument ) );
extern int              top_help;

#define NEW_AUTH(ch) (!IS_NPC(ch) && (ch)->level == 1)
char                    message[MSL];
void free_help( HELP_DATA *pHelp )
{
    if ( !pHelp )
        return;
    if ( pHelp->text )
        STRFREE( pHelp->text );
    if ( pHelp->keyword )
        STRFREE( pHelp->keyword );
    DISPOSE( pHelp );
    return;
}

bool                    check_parse_name( char *name, bool newchar );

/* Keep players from defeating examine progs -Druid
 * False = do not trigger
 * True = Trigger
 */
bool                    EXA_prog_trigger = TRUE;

void save_sysdata       args( ( SYSTEM_DATA sys ) );

void                    colorize_equipment( OBJ_DATA *obj, CHAR_DATA *ch );

const char             *const where_name[] = {
    "&C[used as light    ]&D ",
    "&C[worn on finger   ]&D ",
    "&C[worn on finger   ]&D ",
    "&C[worn around neck ]&D ",
    "&C[worn around neck ]&D ",
    "&C[worn on body     ]&D ",
    "&C[worn on head     ]&D ",
    "&C[worn on legs     ]&D ",
    "&C[worn on feet     ]&D ",
    "&C[worn on hands    ]&D ",
    "&C[worn on arms     ]&D ",
    "&C[worn as shield   ]&D ",
    "&C[worn about body  ]&D ",
    "&C[worn about waist ]&D ",
    "&C[worn around wrist]&D ",
    "&C[worn around wrist]&D ",
    "&C[wielded          ]&D ",
    "&C[held             ]&D ",
    "&C[dual wielded     ]&D ",
    "&C[worn on ears     ]&D ",
    "&C[worn on eyes     ]&D ",
    "&C[missile wielded  ]&D ",
    "&C[worn on back     ]&D ",
    "&C[worn over face   ]&D ",
    "&C[worn around ankle]&D ",
    "&C[worn around ankle]&D ",
    "&C[lodged in a rib  ]&D ",
    "&C[lodged in an arm ]&D ",
    "&C[lodged in a leg  ]&D ",
    "&C[sheath           ]&D ",
    "&C[worn on shoulders]&D "
};

/*
StarMap was written by Nebseni of Clandestine MUD and ported to Smaug
by Desden, el Chaman Tibetano.
*/

#define NUM_DAYS 35
/* Match this to the number of days per month; this is the moon cycle */
#define NUM_MONTHS 17
/* Match this to the number of months defined in month_name[].  */
#define MAP_WIDTH 72
#define MAP_HEIGHT 8
/* Should be the string length and number of the constants below.*/

const char             *star_map[] = {
    "                                               C. C.                  g*",
    "    O:       R*        G*    G.  W* W. W.          C. C.    Y* Y. Y.    ",
    "  O*.                c.          W.W.     W.            C.       Y..Y.  ",
    "O.O. O.              c.  G..G.           W:      B*                   Y.",
    "     O.    c.     c.                     W. W.                  r*    Y.",
    "     O.c.     c.      G.             P..     W.        p.      Y.   Y:  ",
    "        c.                    G*    P.  P.           p.  p:     Y.   Y. ",
    "                 b*             P.: P*                 p.p:             "
};

/****************** CONSTELLATIONS and STARS *****************************
  Cygnus     Mars        Orion      Dragon       Cassiopeia          Venus
           Ursa Ninor                           Mercurius     Pluto    
               Uranus              Leo                Crown       Raptor
*************************************************************************/

const char             *sun_map[] = {
    "\\`|'/",
    "- O -",
    "/.|.\\"
};

const char             *moon_map[] = {
    " @@@ ",
    "@@@@@",
    " @@@ "
};

void look_sky( CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH];
    char                    buf2[4];
    int                     starpos,
                            sunpos,
                            moonpos,
                            moonphase,
                            i,
                            linenum;

    send_to_pager( "You gaze up towards the heavens and see:\r\n", ch );

    struct WeatherCell     *cell = getWeatherCell( ch->in_room->area );

    if ( isModeratelyCloudy( getCloudCover( cell ) ) ) {
        send_to_char( "There are too many clouds in the sky so you cannot see anything else.\r\n",
                      ch );
        return;
    }

    sunpos = ( MAP_WIDTH * ( 24 - time_info.hour ) / 24 );
    moonpos = ( sunpos + time_info.day * MAP_WIDTH / NUM_DAYS ) % MAP_WIDTH;
    if ( ( moonphase =
           ( ( ( ( MAP_WIDTH + moonpos - sunpos ) % MAP_WIDTH ) +
               ( MAP_WIDTH / 16 ) ) * 8 ) / MAP_WIDTH ) > 4 )
        moonphase -= 8;
    starpos = ( sunpos + MAP_WIDTH * time_info.month / NUM_MONTHS ) % MAP_WIDTH;
    /*
     * The left end of the star_map will be straight overhead at midnight during month 0 
     */

    for ( linenum = 0; linenum < MAP_HEIGHT; linenum++ ) {
        if ( ( time_info.hour >= 6 && time_info.hour <= 18 ) && ( linenum < 3 || linenum >= 6 ) )
            continue;

        mudstrlcpy( buf, " ", MAX_STRING_LENGTH );

        /*
         * for( i = MAP_WIDTH/4; i <= 3*MAP_WIDTH/4; i++)
         */
        for ( i = 1; i <= MAP_WIDTH; i++ ) {
            /*
             * plot moon on top of anything else...unless new moon & no eclipse 
             */
            if ( ( time_info.hour >= 6 && time_info.hour <= 18 )    /* daytime? */
                 &&( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 ) /* in
                                                                                             * * *
                                                                                             * *
                                                                                             * sky? 
                                                                                             */
                 &&( i >= moonpos - 2 ) && ( i <= moonpos + 2 ) /* is this pixel near *
                                                                 * moon? */
                 &&( ( sunpos == moonpos && time_info.hour == 12 ) || moonphase != 0 )  /* no 
                                                                                         *
                                                                                         * eclipse */
                 &&( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) ) {
                if ( ( moonphase < 0 && i - 2 - moonpos >= moonphase )
                     || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
                    mudstrlcat( buf, "&W@", MAX_STRING_LENGTH );
                else
                    mudstrlcat( buf, " ", MAX_STRING_LENGTH );
            }
            else if ( ( linenum >= 3 ) && ( linenum < 6 ) &&    /* nighttime */
                      ( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 )  /* in 
                                                                                                 * sky? 
                                                                                                 */
                      &&( i >= moonpos - 2 ) && ( i <= moonpos + 2 )    /* is this pixel
                                                                         * * near moon? */
                      &&( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) ) {
                if ( ( moonphase < 0 && i - 2 - moonpos >= moonphase )
                     || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
                    mudstrlcat( buf, "&W@", MAX_STRING_LENGTH );
                else
                    mudstrlcat( buf, " ", MAX_STRING_LENGTH );
            }
            else {                                     /* plot sun or stars */

                if ( time_info.hour >= 6 && time_info.hour <= 18 ) {    /* daytime */
                    if ( i >= sunpos - 2 && i <= sunpos + 2 ) {
                        snprintf( buf2, 4, "&Y%c", sun_map[linenum - 3][i + 2 - sunpos] );
                        mudstrlcat( buf, buf2, MAX_STRING_LENGTH );
                    }
                    else
                        mudstrlcat( buf, " ", MAX_STRING_LENGTH );
                }
                else {
                    switch ( star_map[linenum][( MAP_WIDTH + i - starpos ) % MAP_WIDTH] ) {
                        default:
                            mudstrlcat( buf, " ", MAX_STRING_LENGTH );
                            break;
                        case ':':
                            mudstrlcat( buf, ":", MAX_STRING_LENGTH );
                            break;
                        case '.':
                            mudstrlcat( buf, ".", MAX_STRING_LENGTH );
                            break;
                        case '*':
                            mudstrlcat( buf, "*", MAX_STRING_LENGTH );
                            break;
                        case 'G':
                            mudstrlcat( buf, "&G ", MAX_STRING_LENGTH );
                            break;
                        case 'g':
                            mudstrlcat( buf, "&g ", MAX_STRING_LENGTH );
                            break;
                        case 'R':
                            mudstrlcat( buf, "&R ", MAX_STRING_LENGTH );
                            break;
                        case 'r':
                            mudstrlcat( buf, "&r ", MAX_STRING_LENGTH );
                            break;
                        case 'C':
                            mudstrlcat( buf, "&C ", MAX_STRING_LENGTH );
                            break;
                        case 'O':
                            mudstrlcat( buf, "&O ", MAX_STRING_LENGTH );
                            break;
                        case 'B':
                            mudstrlcat( buf, "&B ", MAX_STRING_LENGTH );
                            break;
                        case 'P':
                            mudstrlcat( buf, "&P ", MAX_STRING_LENGTH );
                            break;
                        case 'W':
                            mudstrlcat( buf, "&W ", MAX_STRING_LENGTH );
                            break;
                        case 'b':
                            mudstrlcat( buf, "&b ", MAX_STRING_LENGTH );
                            break;
                        case 'p':
                            mudstrlcat( buf, "&p ", MAX_STRING_LENGTH );
                            break;
                        case 'Y':
                            mudstrlcat( buf, "&Y ", MAX_STRING_LENGTH );
                            break;
                        case 'c':
                            mudstrlcat( buf, "&c ", MAX_STRING_LENGTH );
                            break;
                    }
                }
            }
        }
        mudstrlcat( buf, "\r\n", MAX_STRING_LENGTH );
        send_to_pager( buf, ch );
    }
}

char                   *strip_crlf( char *str )
{
    static char             newstr[MSL];
    int                     i,
                            j;

    for ( i = j = 0; str[i] != '\0'; i++ )
        if ( str[i] != '\r' && str[i] != '\n' )
            newstr[j++] = str[i];
    newstr[j] = '\0';
    return newstr;
}

/* Local functions. */
void                    show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch, int num );
void                    show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch );
void                    show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch );
bool                    check_blind( CHAR_DATA *ch );
void                    show_condition( CHAR_DATA *ch, CHAR_DATA *victim );
extern char             str_boot_time[];
extern char             reboot_time[];

char                   *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    static char             buf[MSL];
    bool                    glowsee = FALSE;

    if ( !obj || !ch )
        return ( char * ) "Something is wrong";
    /*
     * can see glowing invis items in the dark 
     */
    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) && IS_OBJ_STAT( obj, ITEM_INVIS )
         && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
        glowsee = TRUE;
    buf[0] = '\0';
    if ( IS_IMMORTAL( ch ) )
        snprintf( buf, MSL, "&C[&c%d&C]&D ", obj->pIndexData->vnum );
    else
        snprintf( buf, MSL, "%s", "&D" );
    /*
     * Sharpen skill --Cronel 
     */
    if ( obj->item_type == ITEM_WEAPON && obj->value[6] > 50 )
        mudstrlcat( buf, "(Sharp) ", MSL );
    if ( IS_OBJ_STAT( obj, ITEM_INVIS ) )
        mudstrlcat( buf, "(Invis) ", MSL );
    if ( ( IS_AFFECTED( ch, AFF_DETECT_EVIL ) ) && IS_OBJ_STAT( obj, ITEM_EVIL ) )
        mudstrlcat( buf, "(Red Aura) ", MSL );
    if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) && IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        mudstrlcat( buf, "(Magical) ", MSL );
    if ( !glowsee && IS_OBJ_STAT( obj, ITEM_GLOW ) )
        mudstrlcat( buf, "(Glowing) ", MSL );
    if ( obj->pIndexData->layers > 0 ) {
        if ( obj->pIndexData->layers == 1 )
            mudstrlcat( buf, "(Layer 1) ", MSL );
        else if ( obj->pIndexData->layers == 2 )
            mudstrlcat( buf, "(Layer 2) ", MSL );
        else if ( obj->pIndexData->layers == 4 )
            mudstrlcat( buf, "(Layer 3) ", MSL );
        else if ( obj->pIndexData->layers == 8 )
            mudstrlcat( buf, "(Layer 4) ", MSL );
        else if ( obj->pIndexData->layers == 16 )
            mudstrlcat( buf, "(Layer 5) ", MSL );
        else if ( obj->pIndexData->layers == 32 )
            mudstrlcat( buf, "(Layer 6) ", MSL );
        else if ( obj->pIndexData->layers == 64 )
            mudstrlcat( buf, "(Layer 7) ", MSL );
        else if ( obj->pIndexData->layers == 128 )
            mudstrlcat( buf, "(Layer 8) ", MSL );
    }
    if ( IS_OBJ_STAT( obj, ITEM_HUM ) )
        mudstrlcat( buf, "(Humming) ", MSL );
    if ( IS_OBJ_STAT( obj, ITEM_HIDDEN ) )
        mudstrlcat( buf, "(Hidden) ", MSL );
    if ( IS_OBJ_STAT( obj, ITEM_BURIED ) ) {
        if ( IS_IMMORTAL( ch ) )
            mudstrlcat( buf, "(Buried) ", MSL );
        else {
            /*
             * EARTHSPEAK! Volk 
             */
            learn_from_success( ch, gsn_earthspeak );
            return ( char * ) "(Earthspeak) A slight tremor alerts you to an object below!";
        }
    }
    if ( IS_IMMORTAL( ch ) && IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
        mudstrlcat( buf, "(PROTO) ", MSL );
    if ( IS_AFFECTED( ch, AFF_DETECTTRAPS ) && is_trapped( obj ) )
        mudstrlcat( buf, "(Trap) ", MSL );
    if ( ( obj->item_type == ITEM_STOVE || obj->item_type == ITEM_FORGE ) && obj->value[0] == 1 )
        mudstrlcat( buf, "(Fired) ", MSL );

    if ( fShort ) {
        if ( glowsee && !IS_IMMORTAL( ch ) )
            mudstrlcat( buf, "the faint glow of something", MSL );
        else if ( obj->short_descr )
            mudstrlcat( buf, obj->short_descr, MSL );
    }
    else {
        if ( glowsee )
            mudstrlcat( buf, "You see the faint glow of something nearby.", MSL );

        if ( obj->description )
            mudstrlcat( buf, obj->description, MSL );
    }
    return buf;
}

/* Some increasingly freaky hallucinated objects  -Thoric
 * (Hats off to Albert Hoffman's "problem child")
 */
const char             *hallucinated_object( int ms, bool fShort )
{
    int                     sms = URANGE( 1, ( ms + 10 ) / 5, 20 );

    if ( fShort )
        switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) ) {
            case 1:
                return "a sword";
            case 2:
                return "a stick";
            case 3:
                return "something shiny";
            case 4:
                return "something";
            case 5:
                return "something interesting";
            case 6:
                return "something colorful";
            case 7:
                return "something that looks cool";
            case 8:
                return "a nifty thing";
            case 9:
                return "a cloak of flowing colors";
            case 10:
                return "a mystical flaming sword";
            case 11:
                return "a swarm of insects";
            case 12:
                return "a deathbane";
            case 13:
                return "a figment of your imagination";
            case 14:
                return "your gravestone";
            case 15:
                return "the long lost boots of Ranger Thoric";
            case 16:
                return "a glowing tome of arcane knowledge";
            case 17:
                return "a long sought secret";
            case 18:
                return "the meaning of it all";
            case 19:
                return "the answer";
            case 20:
                return "the key to life, the universe and everything";
        }
    switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) ) {
        case 1:
            return "A nice looking sword catches your eye.";
        case 2:
            return "The ground is covered in small sticks.";
        case 3:
            return "Something shiny catches your eye.";
        case 4:
            return "Something catches your attention.";
        case 5:
            return "Something interesting catches your eye.";
        case 6:
            return "Something colorful flows by.";
        case 7:
            return "Something that looks cool calls out to you.";
        case 8:
            return "A nifty thing of great importance stands here.";
        case 9:
            return "A cloak of flowing colors asks you to wear it.";
        case 10:
            return "A mystical flaming sword awaits your grasp.";
        case 11:
            return "A swarm of insects buzzes in your face!";
        case 12:
            return "The extremely rare Deathbane lies at your feet.";
        case 13:
            return "A figment of your imagination is at your command.";
        case 14:
            return "You notice a gravestone here... upon closer examination, it reads your name.";
        case 15:
            return "The long lost boots of Ranger Thoric lie off to the side.";
        case 16:
            return "A glowing tome of arcane knowledge hovers in the air before you.";
        case 17:
            return "A long sought secret of all mankind is now clear to you.";
        case 18:
            return "The meaning of it all, so simple, so clear... of course!";
        case 19:
            return "The answer.  One.  It's always been One.";
        case 20:
            return "The key to life, the universe and everything awaits your hand.";
    }
    return "Whoa!!!";
}

/* This is the punct snippet from Desden el Chaman Tibetano - Nov 1998
 * Email: jlalbatros@mx2.redestb.es
 */
char                   *num_punct( int foo )
{
    int                     index_new,
                            rest,
                            x;
    unsigned int            nindex;
    char                    buf[16];
    static char             buf_new[16];

    snprintf( buf, 16, "%d", foo );
    rest = strlen( buf ) % 3;

    for ( nindex = index_new = 0; nindex < strlen( buf ); nindex++, index_new++ ) {
        x = nindex - rest;
        if ( nindex != 0 && ( x % 3 ) == 0 ) {
            buf_new[index_new] = ',';
            index_new++;
            buf_new[index_new] = buf[nindex];
        }
        else
            buf_new[index_new] = buf[nindex];
    }
    buf_new[index_new] = '\0';
    return buf_new;
}

/* Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char                  **prgpstrShow;
    int                    *prgnShow;
    int                    *pitShow;
    int                    *pitColor;
    char                   *pstrShow;
    OBJ_DATA               *obj;
    int                     nShow;
    int                     iShow;
    int                     count,
                            offcount,
                            tmp,
                            ms,
                            cnt;
    bool                    fCombine;

    if ( !ch->desc )
        return;
    /*
     * if there's no list... then don't do all this crap!  -Thoric 
     */
    if ( !list ) {
        if ( fShowNothing ) {
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
                send_to_char( "     ", ch );
            set_char_color( AT_OBJECT, ch );
            send_to_char( "Nothing.\r\n", ch );
        }
        return;
    }
    /*
     * Alloc space for output lines. 
     */
    count = 0;
    for ( obj = list; obj; obj = obj->next_content )
        count++;

/* Volk - dunno why this crashes.. i'll break it down a bit, heh. */

    if ( IS_NPC( ch ) )
        ms = 1;
    else {
        ms = ch->mental_state ? ch->mental_state : 1;
        ms *= ch->pcdata->condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] / 12 ) : 1;
    }

    /*
     * If not mentally stable... 
     */
    if ( abs( ms ) > 40 ) {
        offcount = URANGE( -( count ), ( count * ms ) / 100, count * 2 );
        if ( offcount < 0 )
            offcount += number_range( 0, abs( offcount ) );
        else if ( offcount > 0 )
            offcount -= number_range( 0, offcount );
    }
    else
        offcount = 0;
    if ( count + offcount <= 0 ) {
        if ( fShowNothing ) {
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
                send_to_char( "     ", ch );
            set_char_color( AT_OBJECT, ch );
            send_to_char( "Nothing.\r\n", ch );
        }
        return;
    }
    CREATE( prgpstrShow, char *, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( prgnShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( pitShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( pitColor, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );

    nShow = 0;
    tmp = ( offcount > 0 ) ? offcount : 0;
    cnt = 0;
    /*
     * Format the list of objects. 
     */
    for ( obj = list; obj; obj = obj->next_content ) {
        if ( offcount < 0 && ++cnt > ( count + offcount ) )
            break;
        if ( tmp > 0 && number_bits( 1 ) == 0 ) {
            prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
            prgnShow[nShow] = 1;
            pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
            pitColor[nShow] = obj->color;
            nShow++;
            --tmp;
        }
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
             && ( obj->item_type != ITEM_TRAP || IS_AFFECTED( ch, AFF_DETECTTRAPS ) ) ) {
            pstrShow = format_obj_to_char( obj, ch, fShort );
            fCombine = FALSE;
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) ) {
                /*
                 * Look for duplicates, case sensitive.
                 * * Matches tend to be near end so run loop backwords.
                 */
                for ( iShow = nShow - 1; iShow >= 0; iShow-- ) {
                    if ( !strcmp( prgpstrShow[iShow], pstrShow ) ) {
                        prgnShow[iShow] += obj->count;
                        fCombine = TRUE;
                        break;
                    }
                }
            }
            pitShow[nShow] = obj->item_type;
            /*
             * Couldn't combine, or didn't want to. 
             */
            if ( !fCombine ) {
                prgpstrShow[nShow] = str_dup( pstrShow );
                prgnShow[nShow] = obj->count;
                pitColor[nShow] = obj->color;
                nShow++;
            }
        }
    }
    if ( tmp > 0 ) {
        int                     x;

        for ( x = 0; x < tmp; x++ ) {
            prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
            prgnShow[nShow] = 1;
            pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
            pitColor[nShow] = ( obj ? obj->color : 1 );
            nShow++;
        }
    }
    /*
     * Output the formatted list. -Color support by Thoric 
     */
    for ( iShow = 0; iShow < nShow; iShow++ ) {
        if ( fShowNothing )
            send_to_char( "     ", ch );
        switch ( pitColor[iShow] ) {
            case 0:
                set_char_color( AT_LBLUE, ch );
                break;
            case 1:
                set_char_color( AT_ORANGE, ch );
                break;
            case 2:
                set_char_color( AT_CYAN, ch );
                break;
            case 3:
                set_char_color( AT_RED, ch );
                break;
            case 4:
                set_char_color( AT_MAGIC, ch );
                break;
            case 5:
                set_char_color( AT_WHITE, ch );
                break;
            case 6:
                set_char_color( AT_BLOOD, ch );
                break;
            case 7:
                set_char_color( AT_DBLUE, ch );
                break;
            case 8:
                set_char_color( AT_GREY, ch );
                break;
            case 9:
                set_char_color( AT_GREEN, ch );
                break;
            case 10:
                set_char_color( AT_PINK, ch );
                break;
            case 11:
                set_char_color( AT_DGREEN, ch );
                break;
            case 12:
                set_char_color( AT_PURPLE, ch );
                break;
            case 13:
                set_char_color( AT_DGREY, ch );
                break;
            case 14:
                set_char_color( AT_YELLOW, ch );
                break;
        }
        send_to_char( prgpstrShow[iShow], ch );
        if ( prgnShow[iShow] != 1 )
            ch_printf( ch, " (%d)", prgnShow[iShow] );
        send_to_char( "\r\n", ch );
        DISPOSE( prgpstrShow[iShow] );
    }
    if ( fShowNothing && nShow == 0 ) {
        if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
            send_to_char( "     ", ch );
        send_to_char( "Nothing.\r\n", ch );
    }
    /*
     * Clean up. 
     */
    DISPOSE( prgpstrShow );
    DISPOSE( prgnShow );
    DISPOSE( pitShow );
    DISPOSE( pitColor );
}

void show_list_to_char_pager( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char                  **prgpstrShow;
    int                    *prgnShow;
    int                    *pitShow;
    int                    *pitColor;
    char                   *pstrShow;
    OBJ_DATA               *obj;
    int                     nShow;
    int                     iShow;
    int                     count,
                            offcount,
                            tmp,
                            ms,
                            cnt;
    bool                    fCombine;

    if ( !ch->desc )
        return;
    /*
     * if there's no list... then don't do all this crap!  -Thoric 
     */
    if ( !list ) {
        if ( fShowNothing ) {
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
                send_to_pager( "     ", ch );
            set_pager_color( AT_OBJECT, ch );
            send_to_pager( "Nothing.\r\n", ch );
        }
        return;
    }
    /*
     * Alloc space for output lines. 
     */
    count = 0;
    for ( obj = list; obj; obj = obj->next_content )
        count++;

/* Volk - dunno why this crashes.. i'll break it down a bit, heh. */

    if ( IS_NPC( ch ) )
        ms = 1;
    else {
        ms = ch->mental_state ? ch->mental_state : 1;
        ms *= ch->pcdata->condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] / 12 ) : 1;
    }

    /*
     * If not mentally stable... 
     */
    if ( abs( ms ) > 40 ) {
        offcount = URANGE( -( count ), ( count * ms ) / 100, count * 2 );
        if ( offcount < 0 )
            offcount += number_range( 0, abs( offcount ) );
        else if ( offcount > 0 )
            offcount -= number_range( 0, offcount );
    }
    else
        offcount = 0;
    if ( count + offcount <= 0 ) {
        if ( fShowNothing ) {
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
                send_to_pager( "     ", ch );
            set_pager_color( AT_OBJECT, ch );
            send_to_pager( "Nothing.\r\n", ch );
        }
        return;
    }
    CREATE( prgpstrShow, char *, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( prgnShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( pitShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
    CREATE( pitColor, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );

    nShow = 0;
    tmp = ( offcount > 0 ) ? offcount : 0;
    cnt = 0;
    /*
     * Format the list of objects. 
     */
    for ( obj = list; obj; obj = obj->next_content ) {
        if ( offcount < 0 && ++cnt > ( count + offcount ) )
            break;
        if ( tmp > 0 && number_bits( 1 ) == 0 ) {
            prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
            prgnShow[nShow] = 1;
            pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
            pitColor[nShow] = obj->color;
            nShow++;
            --tmp;
        }
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
             && ( obj->item_type != ITEM_TRAP || IS_AFFECTED( ch, AFF_DETECTTRAPS ) ) ) {
            pstrShow = format_obj_to_char( obj, ch, fShort );
            fCombine = FALSE;
            if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) ) {
                /*
                 * Look for duplicates, case sensitive.
                 * * Matches tend to be near end so run loop backwords.
                 */
                for ( iShow = nShow - 1; iShow >= 0; iShow-- ) {
                    if ( !strcmp( prgpstrShow[iShow], pstrShow ) ) {
                        prgnShow[iShow] += obj->count;
                        fCombine = TRUE;
                        break;
                    }
                }
            }
            pitShow[nShow] = obj->item_type;
            /*
             * Couldn't combine, or didn't want to. 
             */
            if ( !fCombine ) {
                prgpstrShow[nShow] = str_dup( pstrShow );
                prgnShow[nShow] = obj->count;
                pitColor[nShow] = obj->color;
                nShow++;
            }
        }
    }
    if ( tmp > 0 ) {
        int                     x;

        for ( x = 0; x < tmp; x++ ) {
            prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
            prgnShow[nShow] = 1;
            pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
            pitColor[nShow] = ( obj ? obj->color : 1 );
            nShow++;
        }
    }
    /*
     * Output the formatted list. -Color support by Thoric 
     */
    for ( iShow = 0; iShow < nShow; iShow++ ) {
        if ( fShowNothing )
            send_to_pager( "     ", ch );
        switch ( pitColor[iShow] ) {
            case 0:
                set_pager_color( AT_LBLUE, ch );
                break;
            case 1:
                set_pager_color( AT_ORANGE, ch );
                break;
            case 2:
                set_pager_color( AT_CYAN, ch );
                break;
            case 3:
                set_pager_color( AT_RED, ch );
                break;
            case 4:
                set_pager_color( AT_MAGIC, ch );
                break;
            case 5:
                set_pager_color( AT_WHITE, ch );
                break;
            case 6:
                set_pager_color( AT_BLOOD, ch );
                break;
            case 7:
                set_pager_color( AT_DBLUE, ch );
                break;
            case 8:
                set_pager_color( AT_GREY, ch );
                break;
            case 9:
                set_pager_color( AT_GREEN, ch );
                break;
            case 10:
                set_pager_color( AT_PINK, ch );
                break;
            case 11:
                set_pager_color( AT_DGREEN, ch );
                break;
            case 12:
                set_pager_color( AT_PURPLE, ch );
                break;
            case 13:
                set_pager_color( AT_DGREY, ch );
                break;
            case 14:
                set_pager_color( AT_YELLOW, ch );
                break;
        }
        send_to_pager( prgpstrShow[iShow], ch );
        if ( prgnShow[iShow] != 1 )
            pager_printf( ch, " (%d)", prgnShow[iShow] );
        send_to_pager( "\r\n", ch );
        DISPOSE( prgpstrShow[iShow] );
    }
    if ( fShowNothing && nShow == 0 ) {
        if ( IS_NPC( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
            send_to_pager( "     ", ch );
        send_to_pager( "Nothing.\r\n", ch );
    }
    /*
     * Clean up. 
     */
    DISPOSE( prgpstrShow );
    DISPOSE( prgnShow );
    DISPOSE( pitShow );
    DISPOSE( pitColor );
}

/* Show fancy descriptions for certain spell affects  -Thoric */
/* VOLK - Need to take into accunt invis etc. Ch is the looker, victim is being looked at */
void show_visible_affects_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char                    buf[MSL];
    char                    name[MSL];

/* Volk - Money function. Looks good to me. Updated in mud.h to check all names and can_see */

    mudstrlcpy( name, PERS( victim, ch ), MSL );

    name[0] = toupper( name[0] );

    if ( IS_AFFECTED( victim, AFF_DRAGONLORD ) ) {
        ch_printf( ch, "&zThis human appears to cast the shadow of a massive dragon.&D\r\n" );
    }

// Only Sanctuary and Shield
    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_ICESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &W(SANCTUARY)&Y(SHIELD)\r\n", name );
    }
// Only Sanctuary and Fireshield
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && IS_AFFECTED( victim, AFF_SANCTUARY )
         && !IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_ICESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &W(SANCTUARY)&R(FIRESHIELD)\r\n",
                   name );
    }
// Only Sanctuary and Iceshield
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_SANCTUARY )
         && !IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &W(SANCTUARY)&C(ICESHIELD)\r\n",
                   name );
    }
// Only Sanctuary Iceshield and Fireshield
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_SANCTUARY )
         && IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &W(SANCTUARY)&C(ICESHIELD)&R(FIRESHIELD)\r\n",
                   name );
    }
// Only sanctuary and shockshield
    if ( !IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_SANCTUARY )
         && !IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &W(SANCTUARY)&Y(SHOCKSHIELD)\r\n",
                   name );
    }

// Only sanctuary and shockshield and fireshield
    if ( !IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_SANCTUARY )
         && IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &W(SANCTUARY)&Y(SHOCKSHIELD)&R(FIRESHIELD)\r\n",
                   name );
    }

// Only Sanctuary Iceshield Shockshield and Fireshield
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_FIRESHIELD )
         && IS_AFFECTED( victim, AFF_SANCTUARY ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &W(SANCTUARY)&C(ICESHIELD)&R(FIRESHIELD)&Y(SHOCKSHIELD)\r\n",
                   name );
    }

// Only Shield and Iceshield
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_SANCTUARY )
         && IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &Y(SHIELD)&C(ICESHIELD)\r\n", name );
    }
// Only Shield and Fireshield
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) && !IS_AFFECTED( victim, AFF_ICESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &Y(SHIELD)&R(FIRESHIELD)\r\n", name );
    }
// Only Sanctuary Shield Fireshield
    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && IS_AFFECTED( victim, AFF_SHIELD )
         && IS_AFFECTED( victim, AFF_FIRESHIELD ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &W(SANCTUARY)&Y(SHIELD)&R(FIRESHIELD)\r\n",
                   name );
    }
// Only Shield Shockshield
    if ( IS_AFFECTED( victim, AFF_SHIELD ) && IS_AFFECTED( victim, AFF_SHOCKSHIELD )
         && !IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        ch_printf( ch, "&Z%s is surrounded in magical energy. &Y(SHIELD)(SHOCKSHIELD)\r\n", name );
    }
// Only Shield Shockshield Iceshield
    if ( IS_AFFECTED( victim, AFF_SHIELD ) && IS_AFFECTED( victim, AFF_SHOCKSHIELD )
         && IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &Y(SHIELD)&C(ICESHIELD)&Y(SHOCKSHIELD)\r\n",
                   name );
    }

// Only Shield Shockshield Fireshield
    if ( IS_AFFECTED( victim, AFF_SHIELD ) && IS_AFFECTED( victim, AFF_SHOCKSHIELD )
         && !IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &Y(SHIELD)&R(FIRESHIELD)&Y(SHOCKSHIELD)\r\n",
                   name );
    }

// Only Shield Shockshield Fireshield Iceshield
    if ( IS_AFFECTED( victim, AFF_SHIELD ) && IS_AFFECTED( victim, AFF_SHOCKSHIELD )
         && IS_AFFECTED( victim, AFF_ICESHIELD ) && IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        ch_printf( ch,
                   "&Z%s is surrounded in magical energy. &Y(SHIELD)&R(FIRESHIELD)&Y(SHOCKSHIELD)&C(ICESHIELD)\r\n",
                   name );
    }
// Only Sanctuary
    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_ICESHIELD ) ) {
        set_char_color( AT_WHITE, ch );
        if ( IS_GOOD( victim ) )
            ch_printf( ch, "%s glows with an aura of divine radiance.\r\n", name );
        else if ( IS_EVIL( victim ) )
            ch_printf( ch, "&b%s shimmers beneath an aura of dark energy.\r\n", name );
        else
            ch_printf( ch, "&b%s is shrouded in flowing shadow and light.\r\n", name );
    }
// Only Shield
    if ( IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_ICESHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        set_char_color( AT_YELLOW, ch );
        ch_printf( ch, "A glowing sphere surrounds %s's body completely.\r\n", name );
    }
    if ( IS_AFFECTED( victim, AFF_UNHOLY_SPHERE ) ) {
        set_char_color( AT_BLOOD, ch );
        ch_printf( ch, "%s is enclosed in within a unholy sphere.\r\n", name );
    }
    if ( IS_AFFECTED( victim, AFF_SURREAL_SPEED ) ) {
        set_char_color( AT_DBLUE, ch );
        ch_printf( ch, "%s's movements are a blur moving with surreal speed.\r\n", name );
    }
    // Only Fireshield
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( victim, AFF_ICESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        set_char_color( AT_FIRE, ch );
        ch_printf( ch, "%s is engulfed within a blaze of mystical flame.\r\n", name );
    }
// Only Shockshield
    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        set_char_color( AT_YELLOW, ch );
        ch_printf( ch, "%s is surrounded by cascading torrents of energy.\r\n", name );
    }
    if ( IS_AFFECTED( victim, AFF_ACIDMIST ) ) {
        set_char_color( AT_GREEN, ch );
        ch_printf( ch, "%s is visible through a cloud of churning mist.\r\n", name );
    }
// Only Iceshield
    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
         && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_SHIELD )
         && !IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
        set_char_color( AT_LBLUE, ch );
        ch_printf( ch, "%s is encompassed by shards of glistening ice.\r\n", name );
    }
    if ( IS_AFFECTED( victim, AFF_CHARM ) && !xIS_SET( victim->act, ACT_MOUNTABLE ) ) {
        set_char_color( AT_MAGIC, ch );
        ch_printf( ch, "%s wanders in a dazed, zombie-like state.\r\n", name );
    }
    if ( !IS_NPC( victim ) && !victim->desc && victim->switched
         && IS_AFFECTED( victim->switched, AFF_POSSESS ) ) {
        set_char_color( AT_MAGIC, ch );
        mudstrlcpy( buf, PERS( victim, ch ), MSL );
        mudstrlcat( buf, " appears to be in a deep trance...\r\n", MSL );
    }
}

int                     soldier_count;

void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch, int num )
{
    char                    buf[MSL];
    char                    buf1[MSL];
    char                    poof[MSL];

    poof[0] = '\0';
    buf[0] = '\0';

    set_char_color( victim->color, ch );
    if ( IS_NPC( victim ) && IS_IMMORTAL( ch ) )
        snprintf( buf, MSL, "&C[&c%d&C]&D ", victim->pIndexData->vnum );
    else
        snprintf( buf, MSL, "%s", "&D" );

    if ( !IS_NPC( victim ) && !victim->desc ) {
        if ( !victim->switched && !IS_PUPPET( victim ) )
            mudstrlcat( buf, "&P[(Link Dead)] ", MSL );
        else if ( !IS_AFFECTED( victim, AFF_POSSESS )
                  && !IS_SET( victim->pcdata->flags, PCFLAG_PUPPET ) )
            mudstrlcat( buf, "(Switched) ", MSL );
    }
    if ( IS_NPC( victim ) && IS_AFFECTED( victim, AFF_POSSESS ) && IS_IMMORTAL( ch )
         && victim->desc ) {
        snprintf( buf1, MSL, "(%s)", victim->desc->original->name );
        mudstrlcat( buf, buf1, MSL );
    }
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) ) {
        if ( VLD_STR( victim->pcdata->afkbuf ) ) {
            snprintf( buf1, MSL, "[AFK %s] ", victim->pcdata->afkbuf );
            mudstrlcat( buf, buf1, MSL );
        }
        else
            mudstrlcat( buf, "[AFK] ", MSL );
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_RP ) ) {
        mudstrlcat( buf, "&C[&YRP&C]&D ", MSL );
    }

    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_GROUP ) ) {
        mudstrlcat( buf, "&P[&RGROUP MOB&P]&D ", MSL );
    }

    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PRACTICE ) ) {
        mudstrlcat( buf, "&P[&RTEACHER&P]&D ", MSL );
    }
    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_NEWBPRACTICE ) ) {
        mudstrlcat( buf, "&P[&RTEACHER&P]&D ", MSL );
    }

    if ( !IS_NPC( victim ) ) {
        if ( ( xIS_SET( victim->act, PLR_RED ) && xIS_SET( victim->act, PLR_PLAYING ) )
             || xIS_SET( victim->act, PLR_WAITING ) && xIS_SET( victim->act, PLR_RED ) ) {
            mudstrlcat( buf, "&W[&RRED&W]&w ", MSL );
        }
        if ( ( xIS_SET( victim->act, PLR_BLUE ) && xIS_SET( victim->act, PLR_PLAYING ) )
             || xIS_SET( victim->act, PLR_WAITING ) && xIS_SET( victim->act, PLR_BLUE ) ) {
            mudstrlcat( buf, "&W[&CBLUE&W]&w ", MSL );
        }
        if ( xIS_SET( victim->act, PLR_FROZEN ) && xIS_SET( victim->act, PLR_PLAYING ) ) {
            mudstrlcat( buf, "&W[FROZEN]&w ", MSL );
        }
    }

    if ( ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_WIZINVIS ) )
         || ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_MOBINVIS ) ) ) {
        if ( !IS_NPC( victim ) )
            snprintf( buf1, MSL, "(Invis %d) ", victim->pcdata->wizinvis );
        else
            mudstrlcat( buf, "(MobInvis)", MSL );
    }

    set_char_color( victim->color, ch );
    if ( IS_AFFECTED( victim, AFF_INVISIBLE ) )
        mudstrlcat( buf, "(Invis) ", MSL );
    if ( IS_IMMORTAL( victim ) ) {
        if ( !IS_NPC( victim ) )
            mudstrlcat( buf, "&O(&YSTAFF&O)&D ", MSL );
    }
    if ( IS_AFFECTED( victim, AFF_BURROW ) )
        mudstrlcat( buf, "(Burrowed) ", MSL );
    if ( IS_AFFECTED( victim, AFF_HIDE ) )
        mudstrlcat( buf, "(Hide) ", MSL );
    if ( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
        mudstrlcat( buf, "(Translucent) ", MSL );
    if ( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
        mudstrlcat( buf, "(Pink Aura) ", MSL );
    if ( IS_EVIL( victim ) && ( IS_AFFECTED( ch, AFF_DETECT_EVIL ) ) )
        mudstrlcat( buf, "(Red Aura) ", MSL );
    if ( IS_AFFECTED( victim, AFF_BERSERK ) )
        mudstrlcat( buf, "(Wild-eyed) ", MSL );
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_ATTACKER ) )
        mudstrlcat( buf, "(ATTACKER) ", MSL );
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_KILLER ) )
        mudstrlcat( buf, "(KILLER) ", MSL );
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_THIEF ) )
        mudstrlcat( buf, "(THIEF) ", MSL );
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_LITTERBUG ) )
        mudstrlcat( buf, "(LITTERBUG) ", MSL );
    if ( IS_NPC( victim ) && IS_IMMORTAL( ch ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
        mudstrlcat( buf, "(PROTO) ", MSL );
    if ( IS_NPC( victim ) && ch->mount && ch->mount == victim && ch->in_room == ch->mount->in_room )
        mudstrlcat( buf, "(Mount) ", MSL );
    if ( victim->desc && victim->desc->connected == CON_EDITING )
        mudstrlcat( buf, "(Writing) ", MSL );
    if ( victim->morph != NULL && IS_IMMORTAL( ch ) )
        mudstrlcat( buf, "(Morphed) ", MSL );

    set_char_color( victim->color, ch );

    if ( ( victim->position == victim->defposition && victim->long_descr
           && victim->long_descr[0] != '\0' ) || ( victim->morph && victim->morph->morph
                                                   && victim->morph->morph->defpos ==
                                                   victim->position ) ) {
        if ( victim->morph != NULL ) {
            if ( !IS_IMMORTAL( ch ) ) {
                if ( victim->morph->morph != NULL )
                    mudstrlcat( buf, victim->morph->morph->long_desc, MSL );
                else
                    mudstrlcat( buf, victim->long_descr, MSL );
            }
            else {
                mudstrlcat( buf, PERS( victim, ch ), MSL );
            }
        }
        else
            mudstrlcat( buf, ( strip_crlf( victim->long_descr ) ), MSL );
        if ( num > 1 && IS_NPC( victim ) ) {
            snprintf( buf1, MSL, " (%d)", num );
            mudstrlcat( buf, buf1, MSL );
        }
        mudstrlcat( buf, "\r\n", MSL );
        set_char_color( victim->color, ch );
        send_to_char( buf, ch );
        show_visible_affects_to_char( victim, ch );
        return;
    }
    else {
        if ( victim->morph != NULL && victim->morph->morph != NULL && !IS_IMMORTAL( ch ) )
            mudstrlcat( buf, MORPHPERS( victim, ch ), MSL );
        else
            mudstrlcat( buf, PERS( victim, ch ), MSL );
    }
    switch ( victim->position ) {
        case POS_DEAD:
            mudstrlcat( buf, " is DEAD!!", MSL );
            break;

        case POS_MORTAL:
            mudstrlcat( buf, " is mortally wounded.", MSL );
            break;

        case POS_INCAP:
            mudstrlcat( buf, " is incapacitated.", MSL );
            break;

        case POS_STUNNED:
            mudstrlcat( buf, " is lying here stunned.", MSL );
            break;

        case POS_MEDITATING:
            mudstrlcat( buf, " is chanting away here in some kind of meditation.", MSL );
            break;

            /*
             * Furniture ideas taken from ROT
             * * Furniture 1.01 is provided by Xerves
             * * Info rewrite for sleeping/resting/standing/sitting on Objects -- Xerves 
             */
        case POS_SLEEPING:
            if ( victim->on != NULL ) {
                if ( IS_SET( victim->on->value[2], SLEEP_AT ) ) {
                    snprintf( message, MSL, " is sleeping at %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else if ( IS_SET( victim->on->value[2], SLEEP_ON ) ) {
                    snprintf( message, MSL, " is sleeping on %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else {
                    snprintf( message, MSL, " is sleeping in %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
            }
            else {
                if ( ch->position == POS_SITTING || ch->position == POS_RESTING )
                    mudstrlcat( buf, " is sleeping nearby.&G", MSL );
                else
                    mudstrlcat( buf, " is deep in slumber here.&G", MSL );
            }
            break;

        case POS_RESTING:
            if ( victim->on != NULL ) {
                if ( IS_SET( victim->on->value[2], REST_AT ) ) {
                    snprintf( message, MSL, " is resting at %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else if ( IS_SET( victim->on->value[2], REST_ON ) ) {
                    snprintf( message, MSL, " is resting on %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else {
                    snprintf( message, MSL, " is resting in %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
            }
            else {
                if ( ch->position == POS_RESTING )
                    mudstrlcat( buf, " is sprawled out alongside you.&G", MSL );
                else if ( ch->position == POS_MOUNTED )
                    mudstrlcat( buf, " is sprawled out at the foot of your mount.&G", MSL );
                else
                    mudstrlcat( buf, " is sprawled out here.&G", MSL );
            }
            break;

        case POS_SITTING:
            if ( victim->on != NULL && victim->on->short_descr != '\0'
                 && victim->on->value[2] >= 0 ) {
                if ( IS_SET( victim->on->value[2], SIT_AT ) ) {
                    snprintf( message, MSL, " is sitting at %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else if ( IS_SET( victim->on->value[2], SIT_ON ) ) {
                    snprintf( message, MSL, " is sitting on %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else {
                    snprintf( message, MSL, " is sitting in %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
            }
            else
                mudstrlcat( buf, " is sitting here.", MSL );
            break;

        case POS_STANDING:
            if ( victim->on != NULL ) {
                if ( IS_SET( victim->on->value[2], STAND_AT ) ) {
                    snprintf( message, MSL, " is standing at %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else if ( IS_SET( victim->on->value[2], STAND_ON ) ) {
                    snprintf( message, MSL, " is standing on %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
                else {
                    snprintf( message, MSL, " is standing in %s.", victim->on->short_descr );
                    mudstrlcat( buf, message, MSL );
                }
            }
            else if ( IS_IMMORTAL( victim ) )
                mudstrlcat( buf, " is here before you.&G", MSL );

            else if ( ( victim->in_room->sector_type == SECT_UNDERWATER
                        || victim->in_room->sector_type == SECT_OCEANFLOOR )
                      && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !IS_NPC( victim ) )
                mudstrlcat( buf, " is drowning here.", MSL );
/*
      else if( (victim->in_room->sector_type == SECT_UNDERWATER) && !IS_AFFECTED(victim, AFF_AQUA_BREATH) 
&& !IS_NPC(victim))
        mudstrlcat(buf, " is drowning here.&G", MSL);  */

            else if ( victim->in_room->sector_type == SECT_UNDERWATER )
                mudstrlcat( buf, " is here in the water.&G", MSL );
/*
      else if( (victim->in_room->sector_type == SECT_OCEANFLOOR) && !IS_AFFECTED(victim, AFF_AQUA_BREATH) && !IS_NPC(victim))
        mudstrlcat(buf, " is drowning here.&G", MSL);
*/
            else if ( victim->in_room->sector_type == SECT_OCEANFLOOR )
                mudstrlcat( buf, " is standing here in the water.&G", MSL );
            else if ( IS_AFFECTED( victim, AFF_FLOATING ) || IS_AFFECTED( victim, AFF_FLYING ) )
                mudstrlcat( buf, " is hovering here.&G", MSL );
            else if ( IS_AFFECTED( victim, AFF_ROOT ) )
                mudstrlcat( buf, " is standing here, rooted to the ground!&D", MSL );
            else
                mudstrlcat( buf, " is standing here.&D", MSL );
            break;
        case POS_CROUCH:
            mudstrlcat( buf, " is crouched down low here.", MSL );
            break;
        case POS_CRAWL:
            mudstrlcat( buf, " is crawling on the ground here.", MSL );
            break;
        case POS_SHOVE:
            mudstrlcat( buf, " is being shoved around.", MSL );
            break;
        case POS_DRAG:
            mudstrlcat( buf, " is being dragged around.", MSL );
            break;
        case POS_MOUNTED:
            mudstrlcat( buf, " is here, upon ", MSL );
            if ( !victim->mount )
                mudstrlcat( buf, "thin air???", MSL );
            else if ( victim->mount == ch )
                mudstrlcat( buf, "your back.", MSL );
            else if ( victim->in_room == victim->mount->in_room ) {
                mudstrlcat( buf, PERS( victim->mount, ch ), MSL );
                mudstrlcat( buf, ".", MSL );
            }
            else
                mudstrlcat( buf, "someone who left??", MSL );
            break;
        case POS_FIGHTING:
        case POS_EVASIVE:
        case POS_DEFENSIVE:
        case POS_AGGRESSIVE:
        case POS_BERSERK:
            mudstrlcat( buf, " is here, fighting ", MSL );
            if ( !victim->fighting ) {
                mudstrlcat( buf, "thin air???", MSL );
                /*
                 * some bug somewhere.... kinda hackey fix -h 
                 */
                if ( !victim->mount )
                    set_position( victim, POS_STANDING );
                else
                    set_position( victim, POS_MOUNTED );
            }
            else if ( who_fighting( victim ) == ch )
                mudstrlcat( buf, "YOU!", MSL );
            else if ( victim->in_room == victim->fighting->who->in_room ) {
                mudstrlcat( buf, PERS( victim->fighting->who, ch ), MSL );
                mudstrlcat( buf, ".", MSL );
            }
            else
                mudstrlcat( buf, "someone who left??", MSL );
            break;
    }
    mudstrlcat( buf, "\r\n", MSL );
    buf[0] = UPPER( buf[0] );
    send_to_char( buf, ch );
    show_visible_affects_to_char( victim, ch );
    return;
}

void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    int                     iWear;
    bool                    found;

    if ( can_see( victim, ch ) && !IS_NPC( ch ) && !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT );
        if ( victim != ch )
            act( AT_ACTION, "$n looks at $N.", ch, NULL, victim, TO_NOTVICT );
        else
            act( AT_ACTION, "$n looks at $mself.", ch, NULL, victim, TO_NOTVICT );
    }
    if ( VLD_STR( victim->description ) ) {
        if ( victim->morph != NULL && victim->morph->morph != NULL
             && VLD_STR( victim->morph->morph->description ) )
            send_to_char( victim->morph->morph->description, ch );
        else
            send_to_char( victim->description, ch );
    }
    else {
        if ( victim->morph != NULL && victim->morph->morph != NULL
             && VLD_STR( victim->morph->morph->description ) )
            send_to_char( victim->morph->morph->description, ch );
        else if ( IS_NPC( victim ) )
            act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
        else if ( ch != victim )
            act( AT_PLAIN, "$E isn't much to look at...", ch, NULL, victim, TO_CHAR );
        else
            act( AT_PLAIN, "You're not much to look at...", ch, NULL, NULL, TO_CHAR );
    }
    show_race_line( ch, victim );
    show_condition( ch, victim );
    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
        if ( ( obj = get_eq_char( victim, iWear ) ) != NULL && can_see_obj( ch, obj ) ) {
            if ( !found ) {
                send_to_char( "\r\n", ch );
                if ( victim != ch )
                    act( AT_RED, "$N is using:&C", ch, NULL, victim, TO_CHAR );
                else
                    act( AT_RED, "You are using:&C", ch, NULL, NULL, TO_CHAR );
                found = TRUE;
            }
            if ( ( !IS_NPC( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE )
                 && VLD_STR( race_table[victim->race]->where_name[iWear] ) )
                send_to_char( race_table[victim->race]->where_name[iWear], ch );
            else if ( VLD_STR( where_name[iWear] ) )
                send_to_char( where_name[iWear], ch );
            else
                continue;
            colorize_equipment( obj, ch );
            send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
            send_to_char( "\r\n&C", ch );
        }
    }
    /*
     * Crash fix here by Thoric 
     */
    if ( IS_NPC( ch ) || victim == ch )
        return;

    if ( IS_IMMORTAL( ch ) ) {
        if ( IS_NPC( victim ) )
            ch_printf( ch, "\r\nMobile #%d '%s' ", victim->pIndexData->vnum,
                       VLD_STR( victim->name ) ? victim->name : "" );
        else
            ch_printf( ch, "\r\n%s ", VLD_STR( victim->name ) ? victim->name : "" );
        ch_printf( ch, "is a level %d %s %s.\r\n", victim->level,
                   IS_NPC( victim ) ? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
                   npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
                   race_table[victim->race]->race_name &&
                   race_table[victim->race]->race_name[0] != '\0' ?
                   race_table[victim->race]->race_name : "unknown",
                   IS_NPC( victim ) ? victim->Class < MAX_NPC_CLASS
                   && victim->Class >=
                   0 ? npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS
                   && class_table[victim->Class]->who_name
                   && class_table[victim->Class]->who_name[0] !=
                   '\0' ? class_table[victim->Class]->who_name : "unknown" );
    }
    if ( number_percent(  ) < LEARNED( ch, gsn_peek ) ) {
        ch_printf( ch, "\r\nYou peek at %s inventory:\r\n",
                   victim->sex == 1 ? "his" : victim->sex == 2 ? "her" : "its" );
        show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
        learn_from_success( ch, gsn_peek );
    }
    else if ( ch->pcdata->learned[gsn_peek] > 0 )
        learn_from_failure( ch, gsn_peek );
    return;
}

bool is_same_mob( CHAR_DATA *i, CHAR_DATA *j )
{
    if ( !IS_NPC( i ) || !IS_NPC( j ) )
        return FALSE;
    if ( i->pIndexData == j->pIndexData && GET_POS( i ) == GET_POS( j )
         && xSAME_BITS( i->affected_by, j->affected_by ) && xSAME_BITS( i->act, j->act )
         && i->resistant == j->resistant && i->susceptible == j->susceptible
         && i->immune == j->immune && i->spec_fun == j->spec_fun
         && ( ( !VLD_STR( i->name ) && !VLD_STR( j->name ) )
              || ( VLD_STR( i->name ) && VLD_STR( j->name )
                   && !str_cmp( i->name, j->name ) ) ) && ( ( !VLD_STR( i->short_descr )
                                                              && !VLD_STR( j->short_descr ) )
                                                            || ( VLD_STR( i->short_descr )
                                                                 && VLD_STR( j->short_descr )
                                                                 && !str_cmp( i->short_descr,
                                                                              j->short_descr ) ) )
         && ( ( !VLD_STR( i->long_descr ) && !VLD_STR( j->long_descr ) )
              || ( VLD_STR( i->long_descr ) && VLD_STR( j->long_descr )
                   && !str_cmp( i->long_descr, j->long_descr ) ) )
         && ( ( !VLD_STR( i->description ) && !VLD_STR( j->description ) )
              || ( VLD_STR( i->description ) && VLD_STR( j->description )
                   && !str_cmp( i->description, j->description ) ) ) )
        return TRUE;
    return FALSE;
}

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    CHAR_DATA              *rch,
                           *rrch;
    short                   num = 1;

    soldier_count = 0;

    for ( rch = list; rch; rch = rch->next_in_room ) {
        if ( rch == ch || ( rch == supermob && ch->level < LEVEL_IMMORTAL ) )
            continue;

        /*
         * Volk - just because you CAN SEE them doesn't mean you should actually see their
         * long desc - ie darkness and infra 
         */
        if ( can_see( ch, rch )
             && ( !room_is_dark( ch->in_room ) || xIS_SET( ch->act, PLR_HOLYLIGHT ) ) ) {
            if ( IS_NPC( rch )
                 && ( rch->pIndexData->vnum == MOB_VNUM_SOLDIERS
                      || rch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) {
                soldier_count = 1;

                while ( rch->next_in_room ) {
                    if ( !IS_NPC( rch->next_in_room )
                         || ( rch->next_in_room->pIndexData->vnum != MOB_VNUM_SOLDIERS
                              && rch->next_in_room->pIndexData->vnum != MOB_VNUM_ARCHERS ) )
                        break;
                    soldier_count++;
                    rch = rch->next_in_room;
                }
            }
            show_char_to_char_0( rch, ch, soldier_count );
            soldier_count = 0;
        }
        else                                           // Can't see the char so.. can we
                                                       // see ANYTHING?
        {
            if ( room_is_dark( ch->in_room )
                 && ( IS_AFFECTED( ch, AFF_INFRARED ) || IS_AFFECTED( ch, AFF_DEMONIC_SIGHT )
                      || IS_AFFECTED( ch, AFF_TRUESIGHT ) ) ) {
                set_char_color( AT_BLOOD, ch );
                send_to_char( "The red form of a living creature is here.\r\n", ch );
            }
        }

        continue;                                      // For each player
    }
}

bool check_blind( CHAR_DATA *ch )
{
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
        return TRUE;
    if ( IS_AFFECTED( ch, AFF_NOSIGHT ) )
        return TRUE;
    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) ) {
        send_to_char( "You can't see a thing!\r\n", ch );
        return FALSE;
    }
    return TRUE;
}

/* Returns classical DIKU door direction based on text in arg  -Thoric */
int get_door( char *arg )
{
    int                     door;

    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
        door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
        door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
        door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
        door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
        door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
        door = 5;
    else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
        door = 6;
    else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
        door = 7;
    else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
        door = 8;
    else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
        door = 9;
    else
        door = -1;
    return door;
}

//Temporary generator for wilderness names, taking another
//route with this. -Taon.
void gen_wilderness_desc( CHAR_DATA *ch )
{
    if ( WILD_DESC_ON( ch ) ) {
        if ( ch->in_room->description ) {
            send_to_char( ch->in_room->description, ch );
            return;
        }

        switch ( ch->in_room->sector_type ) {
            case SECT_OCEAN:
                send_to_char
                    ( "This magnificent ocean continues beyond the horizons.\r\nThe rolling waves endlessly forming and crashing on\r\nthemselves, the howling wind ever blowing, the sky\r\nseemlessly never ending.\r\n",
                      ch );
                break;
            case SECT_WATERFALL:
                send_to_char
                    ( "This glorious waterfall invokes an awe inspiring sight;\r\nas it plunges into the river below, creating a spray\r\nthat fills the surrounding air.\r\n",
                      ch );
                break;
            case SECT_ARCTIC:
                send_to_char
                    ( "This barren wasteland is made up of nothing more then\r\nsheer ice and snow, which boldly stretches across the\r\nsurrounding landscape.\r\n",
                      ch );
                break;
            case SECT_GRASSLAND:
                send_to_char
                    ( "A grassland covers the entire surrounding landscape\r\nhere. The wild grass growing as thin green stalks\r\nwhich ripple endlessly in the wind.\r\n",
                      ch );
                break;
            case SECT_HIGHMOUNTAIN:
                send_to_char
                    ( "The gawking range of this tall mountain expand over\r\nsurrounding landscape. The peaks stretch beyond the\r\nclouds, nearly into the heavens.\r\n",
                      ch );
                break;
            case SECT_DOCK:
                send_to_char
                    ( "A large, rusty pier stretches out from the shoreline,\r\nproviding a comfortable berth where ships and smaller\r\nboats can dock, before setting off on a new adventure.\r\n",
                      ch );
                break;
            case SECT_LAKE:
                send_to_char
                    ( "The shimmering lake sits fairly calm and appears\r\nquite deep. Ever soothing small waves ripple by as\r\nthe water mills about in complex patterns.\r\n",
                      ch );
                break;
            case SECT_CROSSROAD:
            case SECT_HROAD:
            case SECT_VROAD:
                send_to_char
                    ( "This roadway crosses the landscape here, providing\r\nan excellent place for travelers, armies, merchants\r\nand wagons to traverse the wilderness.\r\n",
                      ch );
                break;
            case SECT_RIVER:
                send_to_char
                    ( "This river travels as a steady stream as it passes\r\nacross the landscape on its way to the ocean.\r\n",
                      ch );
                break;
            case SECT_FOREST:
                send_to_char
                    ( "A forest of trees stands here, rising up into the\r\nsky, sheltering the earth below, allowing an array\r\nof wildlife to exist here.\r\n",
                      ch );
                break;
            case SECT_THICKFOREST:
                send_to_char
                    ( "A thick forest of trees and underbrush surround the\r\nimmediate area, which has spawned a teeming source\r\nof various sorts of wildlife.\r\n",
                      ch );
                break;
            case SECT_HILLS:
                send_to_char
                    ( "Various rounded hilltops cover the landscape here.\r\nTheir rounded peaks left exposed to the wind and\r\nrain, providing ground for tough grass and trees\r\nto grow.\r\n",
                      ch );
                break;
            case SECT_MOUNTAIN:
                send_to_char
                    ( "A series of rocky mountains litter the surrounding\r\nlandscape, with peaks that mearly scrape the clouds.\r\nThis rugged, rutted out rocky terrain doesn't prove\r\neasy to traverse.\r\n",
                      ch );
                break;
            case SECT_AIR:
                send_to_char
                    ( "High above the ground, there are small clouds around here.\r\nEverything appears small on the ground below.  The air is\r\nthinner up in the high altitudes here.",
                      ch );
                break;
            case SECT_CITY:
                send_to_char
                    ( "Before the massive gates of a city.  There is a small\r\ncobblestone layer that extends into the belly of the city.\r\nHooded lanterns are mounted on both sides of the gates.\r\n ",
                      ch );
                break;
            case SECT_DESERT:
                send_to_char
                    ( "Dunes upon dunes of sand sweep the horizon, the\r\nblowing sand throws grit against the few sand-\r\nstone peaks which can be found in this lifeless void.\r\n",
                      ch );
                break;
            case SECT_FIELD:
                send_to_char
                    ( "Mounds upon mounds signal the markings of the fields\r\nthat cover the entire landscape here. This place has\r\nan array of various types of well tended to plants.\r\n",
                      ch );
                break;
            case SECT_SWAMP:
                send_to_char
                    ( "The swampland is moist and quite stagnant here. An\r\neverlasting fog lingers through the area, invoking\r\neerie impluses.\r\n",
                      ch );
                break;
            case SECT_JUNGLE:
                send_to_char
                    ( "A thick, lush foliage covers the entire area, amidst\r\nthe towering trees and tangled vegetation and roots.\r\nInsects are heard buzzing wildly in the humid air of\r\nthe jungle.\r\n",
                      ch );
                break;
            case SECT_CAMPSITE:
                send_to_char
                    ( "There is a patch of soft grass covering the surrounding\r\nlandscape, providing a great place to stop and rest. A\r\ntall tree grows in the middle of the area, giving ample\r\nshade to passing travelers.\r\n",
                      ch );
                break;
            case SECT_AREA_ENT:
                send_to_char
                    ( "The wilderness clears up here.  You may enter or leave the\r\nwilderness here.  The exit to the wilderness appears to enter\r\ninto a new place.\r\n",
                      ch );
                break;
            case SECT_LAVA:
                send_to_char
                    ( "Lava gushes over the entire terrain, burning and scalding\r\nanything it touches. Very few creatures can survive within\r\nits immense heat.\r\n",
                      ch );
                break;
            case SECT_PORTALSTONE:
                send_to_char
                    ( "An ancient stone stands embedded in the ground here.  There\r\nare mystical symbols on the stone side.  A strange looking slot\r\nof the stone appears to be fitted for some object.\r\n",
                      ch );
                break;
            case SECT_DEEPMUD:
                send_to_char
                    ( "The ground is liquidy grained sand that has very little\r\nvegetation growth in it.  A few small creatures scurry on the\r\ntop of the mud.  Some large ruts are in the mud where larger\r\nbeasts have gotten stuck.\r\n",
                      ch );
                break;
            case SECT_QUICKSAND:
                send_to_char
                    ( "The ground is liquidy grained sand that has very little\r\nvegetation growth in it.  There does not appear to be a bottom.\r\nThe more you struggle the deeper you sink.\r\n",
                      ch );
                break;
            case SECT_PASTURELAND:
                send_to_char
                    ( "A vast grassy flatland with livestock animals in the distance.\r\nA large pond of water is in the distance.  Some wild flowers\r\ngrow scattered about the area.\r\n",
                      ch );
                break;
            case SECT_VALLEY:
                send_to_char
                    ( "A beautiful grassy valley between two mountains.  A few\r\nsmall streams come down from the mountains above.\r\nSmall pine trees grow along the valley.\r\n",
                      ch );
                break;
            case SECT_MOUNTAINPASS:
                send_to_char
                    ( "A small passage between two mountains is here.  There are\r\nrocks and boulders along the sides of the passage that\r\n have fallen from above.  The passage is well worn.\r\n",
                      ch );
                break;
            case SECT_BEACH:
                send_to_char
                    ( "The sandy shoreline has an array of various sea shells\r\nalong the beach.  A few breathing holes are in the sand from\r\nhermit crabs.  Seaweed and jellyfish litter the shoreline.\r\n",
                      ch );
                break;
            case SECT_FOG:
                send_to_char
                    ( "A dense fog obscures everything from view.  The ground\r\nis not even visible, nor are your own hands\r\nwhen held at arm length.\r\n",
                      ch );
                break;
            case SECT_SKY:
                send_to_char
                    ( "The clear open sky shows the vastness of the ground below,\r\nand sky around.  Landmarks that cannot be seen on the ground\r\nare easily viewed from the sky.\r\n",
                      ch );
                break;
            case SECT_CLOUD:
                send_to_char
                    ( "The patchy clouds obscure the view of the ground below and\r\nthe sky around.  It is difficult to make out any landmarks.\r\nThe clouds are darker towards the center.\r\n",
                      ch );
                break;
            case SECT_SNOW:
                send_to_char
                    ( "The land is covered in deep hard packed snow here.  There are\r\nsmall trails in the snow from travelers moving through.\r\nLittle signs of animal life exist here.\r\n",
                      ch );
                break;
            case SECT_ORE:
                send_to_char
                    ( "A large collection of raw ore is scattered about.  The ore has\r\na light blue coloring to it, and there are gouges in it where it\r\nhas been mined.  The mining has caused recessing into the ground.\r\n",
                      ch );
                break;
            default:
                log_printf
                    ( "Wilderness Error: Sector out of bounds in %d couldn't establish description.\r\n",
                      ch->in_room->vnum );
                break;
        }
    }
    send_to_char( "\r\n", ch );
    return;
}

//Generate name for rooms in Wilderness area. -Taon
void gen_wilderness_name( CHAR_DATA *ch )
{
    const char             *name;

    switch ( ch->in_room->sector_type ) {
        case SECT_OCEAN:
            name = "The Ocean";
            break;
        case SECT_WATERFALL:
            name = "A Waterfall";
            break;
        case SECT_ARCTIC:
            name = "The Arctic";
            break;
        case SECT_GRASSLAND:
            name = "A Grassland";
            break;
        case SECT_HIGHMOUNTAIN:
            name = "A High Mountain";
            break;
        case SECT_DOCK:
            name = "A Dock";
            break;
        case SECT_CROSSROAD:
            name = "A Crossroad";
            break;
        case SECT_LAKE:
            name = "A Lake";
            break;
        case SECT_HROAD:
        case SECT_VROAD:
            name = "A Roadway";
            break;
        case SECT_RIVER:
            name = "A River";
            break;
        case SECT_FOREST:
            name = "Within a Forest";
            break;
        case SECT_THICKFOREST:
            name = "A Thick Forest";
            break;
        case SECT_HILLS:
            name = "A Hilltop";
            break;
        case SECT_MOUNTAIN:
            name = "A Mountain";
            break;
        case SECT_AREA_ENT:
            name = "Wilderness Access Point";
            break;
        case SECT_CITY:
            name = "Before a City";
            break;
        case SECT_DESERT:
            name = "The Desert";
            break;
        case SECT_FIELD:
            name = "A Field";
            break;
        case SECT_SWAMP:
            name = "A Swamp";
            break;
        case SECT_AIR:
            name = "Up in the Air";
            break;
        case SECT_JUNGLE:
            name = "A Jungle";
            break;
        case SECT_CAMPSITE:
            name = "A Campsite";
            break;
        case SECT_LAVA:
            name = "A Pool of Lava";
            break;
        case SECT_PORTALSTONE:
            name = "A Portal Stone";
            break;
        case SECT_DEEPMUD:
            name = "Within the Deep Mud";
            break;
        case SECT_QUICKSAND:
            name = "Caught Within the Quicksand";
            break;
        case SECT_PASTURELAND:
            name = "Along the Pasture land";
            break;
        case SECT_VALLEY:
            name = "Within a Valley";
            break;
        case SECT_MOUNTAINPASS:
            name = "In a Mountain Pass";
            break;
        case SECT_BEACH:
            name = "On the Sandy Beach";
            break;
        case SECT_FOG:
            name = "Within the Dense Fog";
            break;
        case SECT_SKY:
            name = "Within the Vast Sky";
            break;
        case SECT_CLOUD:
            name = "Within some Patchy Clouds";
            break;
        case SECT_SNOW:
            name = "Within Snow Covered Lands";
            break;
        case SECT_ORE:
            name = "Within an Ore Quarry";
            break;
        default:
            log_printf( "Wilderness Error: Sector out of bounds in %d, couldn't establish name. ",
                        ch->in_room->vnum );
            name = "Sector Undefined";
            break;
    }

    if ( WILD_NAME_ON( ch ) )
        send_to_char( name, ch );
}

void do_look( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    EXIT_DATA              *pexit;
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    ROOM_INDEX_DATA        *original;
    char                   *pdesc;
    short                   door;
    int                     number,
                            cnt;

    if ( !ch->desc )
        return;

    if ( ch->position < POS_SLEEPING ) {
        send_to_char( "You can't see anything but stars!\r\n", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
        return;
    }

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are concentrating too much for that!\r\n", ch );
        return;
    }

    if ( !check_blind( ch ) )
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) ) {
        /*
         * Room flag display installed by Samson 12-10-97 
         */
        if ( IS_IMMORTAL( ch ) && xIS_SET( ch->act, PLR_STAFF ) ) {
            ch_printf( ch, "&w[&cArea Flags:&C %s &w]",
                       flag_string( ch->in_room->area->flags, area_flags ) );
            ch_printf( ch, "[&cRoom Flags: &C%s&w]\r\n",
                       flag_string( ch->in_room->room_flags, r_flags ) );
            ch_printf( ch, "[&cSector Type: &C%s&w]", sec_flags[ch->in_room->sector_type] );
            ch_printf( ch, "[&cArea name: &C%s&w]  ", ch->in_room->area->name );
            ch_printf( ch, "\r\n[&cArea filename: &C%s&w]", ch->in_room->area->filename );
            ch_printf( ch, "[&cRoom vnum: &C#%d&w ]&W\r\n", ch->in_room->vnum );
        }

        set_char_color( AT_RMNAME, ch );

        /*
         * 'look' or 'look auto' 
         */

        bool                    found = FALSE;

        if ( IN_WILDERNESS( ch ) || IN_RIFT( ch ) ) {
            if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) || IS_AFFECTED( ch, AFF_DEMONIC_SIGHT ) && ( !IS_BLIND( ch ) ) ) {    /* diamond 
                                                                                                                         */
                found = TRUE;
                ch->map_size = 2;
                ch->map_type = 1;
            }
            if ( IS_AFFECTED( ch, AFF_WIZARD_SIGHT ) && ( !IS_BLIND( ch ) ) ) { /* sphere 
                                                                                 */
                found = TRUE;
                ch->map_size = 2;
                ch->map_type = 2;
            }
            if ( IS_AFFECTED( ch, AFF_BATTLEFIELD ) && ( !IS_BLIND( ch ) ) ) {
                found = TRUE;
                ch->map_type = 0;
            }

            if ( !found )
                ch->map_type = 0;
            gen_wilderness_name( ch );

            if ( VLD_STR( ch->landmark ) )
                do_landmark( ch, ch->landmark );
        }
        else
            send_to_char( ch->in_room->name, ch );

        send_to_char( "\r\n", ch );

/* Volk - moved dark code here so infra can still see room name! */

        if ( !IS_NPC( ch )
             && !xIS_SET( ch->act, PLR_HOLYLIGHT )
             && !IS_AFFECTED( ch, AFF_DEMONIC_SIGHT ) && !IS_AFFECTED( ch, AFF_TRUESIGHT )
             && ( ( IS_SET( ch->in_room->area->flags, AFLAG_DARKNESS ) && ch->in_room->light <= 0 )
                  || room_is_dark( ch->in_room ) ) ) {
            set_char_color( AT_DGREY, ch );
            send_to_char( "It is pitch black ... \r\n", ch );
            show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
            show_char_to_char( ch->in_room->first_person, ch );
            return;
        }

//    set_char_color(AT_RMDESC, ch);
        if ( ch->in_room->area->color == 0 )
            set_char_color( AT_LBLUE, ch );
        else if ( ch->in_room->area->color == 1 )
            set_char_color( AT_ORANGE, ch );
        else if ( ch->in_room->area->color == 2 )
            set_char_color( AT_CYAN, ch );
        else if ( ch->in_room->area->color == 3 )
            set_char_color( AT_RED, ch );
        else if ( ch->in_room->area->color == 4 )
            set_char_color( AT_MAGIC, ch );
        else if ( ch->in_room->area->color == 5 )
            set_char_color( AT_WHITE, ch );
        else if ( ch->in_room->area->color == 6 )
            set_char_color( AT_BLOOD, ch );
        else if ( ch->in_room->area->color == 7 )
            set_char_color( AT_DBLUE, ch );
        else if ( ch->in_room->area->color == 8 )
            set_char_color( AT_GREY, ch );
        else if ( ch->in_room->area->color == 9 )
            set_char_color( AT_GREEN, ch );
        else if ( ch->in_room->area->color == 10 )
            set_char_color( AT_PINK, ch );
        else if ( ch->in_room->area->color == 11 )
            set_char_color( AT_DGREEN, ch );
        else if ( ch->in_room->area->color == 12 )
            set_char_color( AT_PURPLE, ch );
        else if ( ch->in_room->area->color == 13 )
            set_char_color( AT_DGREY, ch );
        else if ( ch->in_room->area->color == 14 )
            set_char_color( AT_YELLOW, ch );

        if ( arg1[0] == '\0' || ( !IS_NPC( ch ) && !xIS_SET( ch->act, PLR_BRIEF ) ) )
            if ( IN_WILDERNESS( ch ) || IN_RIFT( ch ) )
                gen_wilderness_desc( ch );
            else
                send_to_char( ch->in_room->description, ch );

        if ( ( IN_WILDERNESS( ch ) || IN_RIFT( ch ) ) && WILD_MAP_ON( ch ) && !IS_BLIND( ch ) )
            do_lookmap( ch, ( char * ) "auto" );

        set_char_color( AT_EXITS, ch );
        if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_AUTOEXIT ) )
            do_exits( ch, ( char * ) "auto" );
        set_char_color( AT_OBJECT, ch );
        show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );

        show_char_to_char( ch->in_room->first_person, ch );
        return;
    }

    if ( !str_cmp( arg1, "sky" ) || !str_cmp( arg1, "stars" ) ) {
        if ( !IS_OUTSIDE( ch ) )
            send_to_char( "You can't see the sky indoors.\r\n", ch );
        else
            look_sky( ch );
        return;
    }

    if ( !str_cmp( arg1, "under" ) ) {
        int                     count;

        /*
         * 'look under' 
         */
        if ( arg2[0] == '\0' ) {
            send_to_char( "Look beneath what?\r\n", ch );
            return;
        }

        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL ) {
            send_to_char( "You do not see that here.\r\n", ch );
            return;
        }
        if ( !CAN_WEAR( obj, ITEM_TAKE ) && ch->level < sysdata.level_getobjnotake ) {
            send_to_char( "You can't seem to get a grip on it.\r\n", ch );
            return;
        }
        if ( ch->carry_weight + obj->weight > can_carry_w( ch ) ) {
            send_to_char( "It's too heavy for you to look under.\r\n", ch );
            return;
        }
        count = obj->count;
        obj->count = 1;
        act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL, TO_CHAR );
        act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL, TO_ROOM );
        obj->count = count;
        if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
            show_list_to_char( obj->first_content, ch, TRUE, TRUE );
        else
            send_to_char( "Nothing.\r\n", ch );
        if ( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
        return;
    }

    if ( !str_cmp( arg1, "a" ) || !str_cmp( arg1, "at" ) ) {
        if ( arg2[0] == '\0' ) {
            send_to_char( "look at what?\r\n", ch );
            return;
        }

        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL ) {
            send_to_char( "You do not see that here.\r\n", ch );
            return;
        }

        pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
        if ( !pdesc )
            pdesc = get_extra_descr( obj->name, obj->first_extradesc );
        if ( !pdesc )
            send_to_char( "There are no seals or engravements upon it.\r\n", ch );
        else
            send_to_char( pdesc, ch );

        if ( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );

        return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) ) {
        int                     count;

        /*
         * 'look in' 
         */
        if ( arg2[0] == '\0' ) {
            send_to_char( "Look in what?\r\n", ch );
            return;
        }

        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL ) {
            send_to_char( "You do not see that here.\r\n", ch );
            return;
        }

        switch ( obj->item_type ) {
            default:
                send_to_char( "That is not a container.\r\n", ch );
                break;

            case ITEM_DRINK_CON:
                if ( obj->value[1] <= 0 ) {
                    send_to_char( "It is empty.\r\n", ch );
                    if ( EXA_prog_trigger )
                        oprog_examine_trigger( ch, obj );
                    break;
                }

                ch_printf( ch, "It's %s full of a %s liquid.\r\n",
                           obj->value[1] < obj->value[0] / 4 ? "less than" : obj->value[1] <
                           3 * obj->value[0] / 4 ? "about" : "more than",
                           liq_table[obj->value[2]].liq_color );

                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                break;

            case ITEM_PORTAL:
                for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
                    if ( pexit->vdir == DIR_PORTAL && IS_SET( pexit->exit_info, EX_PORTAL ) ) {
                        if ( room_is_private( pexit->to_room )
                             && get_trust( ch ) < sysdata.level_override_private ) {
                            set_char_color( AT_WHITE, ch );
                            send_to_char( "That room is private buster!\r\n", ch );
                            return;
                        }
                        original = ch->in_room;
                        char_from_room( ch );
                        char_to_room( ch, pexit->to_room );
                        do_look( ch, ( char * ) "auto" );
                        char_from_room( ch );
                        char_to_room( ch, original );
                        return;
                    }
                }
                send_to_char( "You see swirling chaos...\r\n", ch );
                break;

            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                if ( obj->value[5] == 1 ) {
                    send_to_char( "That is not a container.\r\n", ch );
                    break;
                }

                if ( !obj->first_content ) {
                    send_to_char( "You see nothing in the corpse.\r\n", ch );
                    break;
                }

            case ITEM_CONTAINER:
            case ITEM_QUIVER:
                if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
                    send_to_char( "It is closed.\r\n", ch );
                    break;
                }

            case ITEM_KEYRING:
                count = obj->count;
                obj->count = 1;
                if ( obj->item_type == ITEM_CONTAINER )
                    act( AT_OBJECT, "$p contains:", ch, obj, NULL, TO_CHAR );
                else
                    act( AT_OBJECT, "$p holds:", ch, obj, NULL, TO_CHAR );
                obj->count = count;
                show_list_to_char( obj->first_content, ch, TRUE, TRUE );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                break;

            case ITEM_SHEATH:
                break;
        }
        return;
    }

    if ( ( pdesc = get_extra_descr( arg1, ch->in_room->first_extradesc ) ) != NULL ) {
        send_to_char( pdesc, ch );
        return;
    }

    door = get_door( arg1 );
    if ( ( pexit = find_door( ch, arg1, TRUE ) ) != NULL ) {
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) && !IS_SET( pexit->exit_info, EX_WINDOW ) ) {
            if ( ( IS_SET( pexit->exit_info, EX_SECRET ) || IS_SET( pexit->exit_info, EX_DIG ) )
                 && door != -1 )
                send_to_char( "Nothing special there.\r\n", ch );
            else
                act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_BASHED ) )
            act( AT_RED, "The $d has been bashed from its hinges!", ch, NULL, pexit->keyword,
                 TO_CHAR );

        if ( pexit->description && pexit->description[0] != '\0' )
            send_to_char( pexit->description, ch );
        else
            send_to_char( "Nothing special there.\r\n", ch );

        /*
         * Ability to look into the next room   -Thoric
         */
        if ( pexit->to_room
             && ( IS_AFFECTED( ch, AFF_SCRYING ) || IS_SET( pexit->exit_info, EX_xLOOK )
                  || get_trust( ch ) >= LEVEL_IMMORTAL ) ) {
            if ( !IS_SET( pexit->exit_info, EX_xLOOK ) && get_trust( ch ) < LEVEL_IMMORTAL ) {
                set_char_color( AT_MAGIC, ch );
                send_to_char( "You attempt to scry...\r\n", ch );
                /*
                 * Change by Narn, Sept 96 to allow characters who don't have the
                 * scry spell to benefit from objects that are affected by scry.
                 */
                if ( !IS_NPC( ch ) ) {
                    int                     percent = LEARNED( ch, skill_lookup( "scry" ) );

                    if ( !percent ) {
                        percent = 65;                  /* 95 was too good -Thoric */
                    }

                    if ( number_percent(  ) > percent ) {
                        send_to_char( "You fail.\r\n", ch );
                        return;
                    }
                }
            }
            if ( room_is_private( pexit->to_room )
                 && get_trust( ch ) < sysdata.level_override_private ) {
                set_char_color( AT_WHITE, ch );
                send_to_char( "That room is private buster!\r\n", ch );
                return;
            }
            original = ch->in_room;
            char_from_room( ch );
            char_to_room( ch, pexit->to_room );
            do_look( ch, ( char * ) "auto" );
            char_from_room( ch );
            char_to_room( ch, original );
        }
        return;
    }
    else if ( door != -1 ) {
        send_to_char( "Nothing special there.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL ) {
        show_char_to_char_1( victim, ch );
        return;
    }

    /*
     * finally fixed the annoying look 2.obj desc bug  -Thoric 
     */
    number = number_argument( arg1, arg );
    for ( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content ) {
        if ( can_see_obj( ch, obj ) ) {
            if ( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) != NULL ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }

            if ( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) != NULL ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }
            if ( nifty_is_name_prefix( arg, obj->name ) ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
                if ( !pdesc )
                    pdesc = get_extra_descr( obj->name, obj->first_extradesc );
                if ( !pdesc )
                    send_to_char( "You see nothing special.\r\n", ch );
                else
                    send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }
        }
    }

    for ( obj = ch->in_room->last_content; obj; obj = obj->prev_content ) {
        if ( can_see_obj( ch, obj ) ) {
            if ( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) != NULL ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }

            if ( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) != NULL ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }
            if ( nifty_is_name_prefix( arg, obj->name ) ) {
                if ( ( cnt += obj->count ) < number )
                    continue;
                pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
                if ( !pdesc )
                    pdesc = get_extra_descr( obj->name, obj->first_extradesc );
                if ( !pdesc )
                    send_to_char( "You see nothing special.\r\n", ch );
                else
                    send_to_char( pdesc, ch );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                return;
            }
        }
    }

    send_to_char( "You do not see that here.\r\n", ch );
    return;
}

void show_race_line( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     feet,
                            inches;

    if ( !IS_NPC( victim ) && ( victim != ch ) ) {
        feet = victim->height / 12;
        inches = victim->height % 12;
        ch_printf( ch, "%s is %d'%d\" and weighs %d pounds.\r\n", PERS( victim, ch ), feet, inches,
                   victim->weight );
        return;
    }
    if ( !IS_NPC( victim ) && ( victim == ch ) ) {
        feet = victim->height / 12;
        inches = victim->height % 12;
        ch_printf( ch, "You are %d'%d\" and weigh %d pounds.\r\n", feet, inches, victim->weight );
        return;
    }

}

void show_condition( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char                    buf[MSL];
    int                     percent;

    if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
    else
        percent = -1;

    if ( victim != ch )
    {
        mudstrlcpy( buf, PERS( victim, ch ), MSL );
        if ( percent >= 100 )
            mudstrlcat( buf, " is in perfect health.\r\n", MSL );
        else if ( percent >= 90 )
            mudstrlcat( buf, " is slightly scratched.\r\n", MSL );
        else if ( percent >= 80 )
            mudstrlcat( buf, " has a few bruises.\r\n", MSL );
        else if ( percent >= 70 )
            mudstrlcat( buf, " has some cuts.\r\n", MSL );
        else if ( percent >= 60 )
            mudstrlcat( buf, " has several wounds.\r\n", MSL );
        else if ( percent >= 50 )
            mudstrlcat( buf, " has many nasty wounds.\r\n", MSL );
        else if ( percent >= 40 )
            mudstrlcat( buf, " is bleeding freely.\r\n", MSL );
        else if ( percent >= 30 )
            mudstrlcat( buf, " is covered in blood.\r\n", MSL );
        else if ( percent >= 20 )
            mudstrlcat( buf, " is leaking guts.\r\n", MSL );
        else if ( percent >= 10 )
            mudstrlcat( buf, " is almost dead.\r\n", MSL );
        else
            mudstrlcat( buf, " is DYING.\r\n", MSL );
    }
    else {
        mudstrlcpy( buf, "You", MSL );
        if ( percent >= 100 )
            mudstrlcat( buf, " are in perfect health.\r\n", MSL );
        else if ( percent >= 90 )
            mudstrlcat( buf, " are slightly scratched.\r\n", MSL );
        else if ( percent >= 80 )
            mudstrlcat( buf, " have a few bruises.\r\n", MSL );
        else if ( percent >= 70 )
            mudstrlcat( buf, " have some cuts.\r\n", MSL );
        else if ( percent >= 60 )
            mudstrlcat( buf, " have several wounds.\r\n", MSL );
        else if ( percent >= 50 )
            mudstrlcat( buf, " have many nasty wounds.\r\n", MSL );
        else if ( percent >= 40 )
            mudstrlcat( buf, " are bleeding freely.\r\n", MSL );
        else if ( percent >= 30 )
            mudstrlcat( buf, " are covered in blood.\r\n", MSL );
        else if ( percent >= 20 )
            mudstrlcat( buf, " are leaking guts.\r\n", MSL );
        else if ( percent >= 10 )
            mudstrlcat( buf, " are almost dead.\r\n", MSL );
        else
            mudstrlcat( buf, " are DYING.\r\n", MSL );
    }

    buf[0] = UPPER( buf[0] );
    send_to_char( buf, ch );
    return;
}

/* A much simpler version of look, this function will show you only
the condition of a mob or pc, or if used without an argument, the
same you would see if you enter the room and have config +brief.
-- Narn, winter '96
*/
void do_glance( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    CHAR_DATA              *victim;
    bool                    brief;

    if ( !ch->desc )
        return;

    if ( ch->position < POS_SLEEPING ) {
        send_to_char( "You can't see anything but stars!\r\n", ch );
        return;
    }

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are concentrating too hard for that.\r\n", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
        return;
    }

    if ( !check_blind( ch ) )
        return;

    set_char_color( AT_ACTION, ch );
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        if ( xIS_SET( ch->act, PLR_BRIEF ) )
            brief = TRUE;
        else
            brief = FALSE;
        xSET_BIT( ch->act, PLR_BRIEF );
        do_look( ch, ( char * ) "auto" );
        if ( !brief )
            xREMOVE_BIT( ch->act, PLR_BRIEF );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "They're not here.\r\n", ch );
        return;
    }
    else {
        if ( can_see( victim, ch ) ) {
            act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT );
            act( AT_ACTION, "$n glances at $N.", ch, NULL, victim, TO_NOTVICT );
        }
        if ( IS_IMMORTAL( ch ) && victim != ch ) {
            if ( IS_NPC( victim ) )
                ch_printf( ch, "Mobile #%d '%s' ", victim->pIndexData->vnum, victim->name );
            else
                ch_printf( ch, "%s ", PERS( victim, ch ) );
            /*
             * ch_printf(ch, "%s ", victim->name);  
             */
            ch_printf( ch, "is a level %d %s %s.\r\n",
                       victim->level,
                       IS_NPC( victim ) ? victim->race < MAX_NPC_RACE
                       && victim->race >=
                       0 ? npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE
                       && race_table[victim->race]->race_name
                       && race_table[victim->race]->race_name[0] !=
                       '\0' ? race_table[victim->race]->race_name : "unknown",
                       IS_NPC( victim ) ? victim->Class < MAX_NPC_CLASS
                       && victim->Class >=
                       0 ? npc_class[victim->Class] : "unknown" : victim->Class <
                       MAX_PC_CLASS && class_table[victim->Class]->who_name
                       && class_table[victim->Class]->who_name[0] !=
                       '\0' ? class_table[victim->Class]->who_name : "unknown" );
/* New Change
      victim->race<MAX_NPC_RACE&&victim->race>=0?npc_race[victim->race] : "unknown",
      victim->class<MAX_NPC_CLASS&&victim->Class>=0?npc_class[victim->Class] : "unknown");
*/
        }
        show_condition( ch, victim );

        return;
    }

    return;
}

void do_examine( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    OBJ_DATA               *obj;
    short                   dam;

    if ( !argument ) {
        bug( "%s", "do_examine: null argument." );
        return;
    }

    if ( !ch ) {
        bug( "%s", "do_examine: null ch." );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Examine what?\r\n", ch );
        return;
    }

    EXA_prog_trigger = FALSE;
//do_look(ch, buf);
    EXA_prog_trigger = TRUE;

    /*
     * Support for looking at boards, checking equipment conditions,
     * and support for trigger positions by Thoric
     */
    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {

        switch ( obj->item_type ) {
            default:
                break;

            case ITEM_ARMOR:
                if ( obj->value[1] == 0 )
                    obj->value[1] = obj->value[0];
                if ( obj->value[1] == 0 )
                    obj->value[1] = 1;
                dam = ( short ) ( ( obj->value[0] * 10 ) / obj->value[1] );
                mudstrlcpy( buf, "As you look more closely, you notice that it is ", MSL );
                if ( dam >= 10 )
                    mudstrlcat( buf, "in superb condition.", MSL );
                else if ( dam == 9 )
                    mudstrlcat( buf, "in very good condition.", MSL );
                else if ( dam == 8 )
                    mudstrlcat( buf, "in good shape.", MSL );
                else if ( dam == 7 )
                    mudstrlcat( buf, "showing a bit of wear.", MSL );
                else if ( dam == 6 )
                    mudstrlcat( buf, "a little run down.", MSL );
                else if ( dam == 5 )
                    mudstrlcat( buf, "in need of repair.", MSL );
                else if ( dam == 4 )
                    mudstrlcat( buf, "in great need of repair.", MSL );
                else if ( dam == 3 )
                    mudstrlcat( buf, "in dire need of repair.", MSL );
                else if ( dam == 2 )
                    mudstrlcat( buf, "very badly worn.", MSL );
                else if ( dam == 1 )
                    mudstrlcat( buf, "practically worthless.", MSL );
                else if ( dam <= 0 )
                    mudstrlcat( buf, "broken.", MSL );
                mudstrlcat( buf, "\r\n", MSL );
                send_to_char( buf, ch );
                break;

            case ITEM_WEAPON:
                dam = INIT_WEAPON_CONDITION - obj->value[0];
                mudstrlcpy( buf, "As you look more closely, you notice that it is ", MSL );
                if ( dam == 0 )
                    mudstrlcat( buf, "in superb condition.", MSL );
                else if ( dam == 1 )
                    mudstrlcat( buf, "in excellent condition.", MSL );
                else if ( dam == 2 )
                    mudstrlcat( buf, "in very good condition.", MSL );
                else if ( dam == 3 )
                    mudstrlcat( buf, "in good shape.", MSL );
                else if ( dam == 4 )
                    mudstrlcat( buf, "showing a bit of wear.", MSL );
                else if ( dam == 5 )
                    mudstrlcat( buf, "a little run down.", MSL );
                else if ( dam == 6 )
                    mudstrlcat( buf, "in need of repair.", MSL );
                else if ( dam == 7 )
                    mudstrlcat( buf, "in great need of repair.", MSL );
                else if ( dam == 8 )
                    mudstrlcat( buf, "in dire need of repair.", MSL );
                else if ( dam == 9 )
                    mudstrlcat( buf, "very badly worn.", MSL );
                else if ( dam == 10 )
                    mudstrlcat( buf, "practically worthless.", MSL );
                else if ( dam == 11 )
                    mudstrlcat( buf, "almost broken.", MSL );
                else if ( dam == 12 )
                    mudstrlcat( buf, "broken.", MSL );
                mudstrlcat( buf, "\r\n", MSL );
                send_to_char( buf, ch );
                break;

            case ITEM_COOK:
                mudstrlcpy( buf, "As you examine it carefully you notice that it ", MSL );
                dam = obj->value[2];
                if ( dam >= 3 )
                    mudstrlcat( buf, "is burned to a crisp.", MSL );
                else if ( dam == 2 )
                    mudstrlcat( buf, "is a little over cooked.", MSL );
                else if ( dam == 1 )
                    mudstrlcat( buf, "is perfectly roasted.", MSL );
                else
                    mudstrlcat( buf, "is raw.", MSL );
                mudstrlcat( buf, "\r\n", MSL );
                send_to_char( buf, ch );
            case ITEM_FOOD:
                if ( obj->timer > 0 && obj->value[1] > 0 )
                    dam = ( obj->timer * 10 ) / obj->value[1];
                else
                    dam = 10;
                if ( obj->item_type == ITEM_FOOD )
                    mudstrlcpy( buf, "As you examine it carefully you notice that it ", MSL );
                else
                    mudstrlcpy( buf, "Also it ", MSL );
                if ( dam >= 10 )
                    mudstrlcat( buf, "is fresh.", MSL );
                else if ( dam == 9 )
                    mudstrlcat( buf, "is nearly fresh.", MSL );
                else if ( dam == 8 )
                    mudstrlcat( buf, "is perfectly fine.", MSL );
                else if ( dam == 7 )
                    mudstrlcat( buf, "looks good.", MSL );
                else if ( dam == 6 )
                    mudstrlcat( buf, "looks ok.", MSL );
                else if ( dam == 5 )
                    mudstrlcat( buf, "is a little stale.", MSL );
                else if ( dam == 4 )
                    mudstrlcat( buf, "is a bit stale.", MSL );
                else if ( dam == 3 )
                    mudstrlcat( buf, "smells slightly off.", MSL );
                else if ( dam == 2 )
                    mudstrlcat( buf, "smells quite rank.", MSL );
                else if ( dam == 1 )
                    mudstrlcat( buf, "smells revolting!", MSL );
                else if ( dam <= 0 )
                    mudstrlcat( buf, "is crawling with maggots!", MSL );
                mudstrlcat( buf, "\r\n", MSL );
                send_to_char( buf, ch );
                break;

            case ITEM_SWITCH:
            case ITEM_LEVER:
            case ITEM_PULLCHAIN:
                if ( IS_SET( obj->value[0], TRIG_UP ) )
                    send_to_char( "You notice that it is in the up position.\r\n", ch );
                else
                    send_to_char( "You notice that it is in the down position.\r\n", ch );
                break;
            case ITEM_BUTTON:
                if ( IS_SET( obj->value[0], TRIG_UP ) )
                    send_to_char( "You notice that it is depressed.\r\n", ch );
                else
                    send_to_char( "You notice that it is not depressed.\r\n", ch );
                break;
            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
                {
                    short                   timerfrac = obj->timer;

                    if ( obj->item_type == ITEM_CORPSE_PC )
                        timerfrac = ( int ) obj->timer / 8 + 1;

                    switch ( timerfrac ) {
                        default:
                            send_to_char( "This corpse has recently been slain.\r\n", ch );
                            break;
                        case 4:
                            send_to_char( "This corpse was slain a little while ago.\r\n", ch );
                            break;
                        case 3:
                            send_to_char
                                ( "A foul smell rises from the corpse, and it is covered in flies.\r\n",
                                  ch );
                            break;
                        case 2:
                            send_to_char
                                ( "A writhing mass of maggots and decay, you can barely go near this corpse.\r\n",
                                  ch );
                            break;
                        case 1:
                        case 0:
                            send_to_char
                                ( "Little more than bones, there isn't much left of this corpse.\r\n",
                                  ch );
                            break;
                    }
                }
            case ITEM_CONTAINER:
                if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
                    break;

                if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
                    send_to_char( "It is closed.\r\n", ch );
                    return;
                }

                int                     count;

                EXA_prog_trigger = FALSE;
                count = obj->count;
                obj->count = 1;
                pager_printf( ch, "When you look inside %s, you see:\r\n",
                              can_see_obj( ch, obj ) ? obj_short( obj ) : "something" );
                obj->count = count;
                show_list_to_char_pager( obj->first_content, ch, TRUE, TRUE );
                if ( EXA_prog_trigger )
                    oprog_examine_trigger( ch, obj );
                EXA_prog_trigger = TRUE;

                break;

            case ITEM_DRINK_CON:
            case ITEM_QUIVER:
                send_to_char( "When you look inside, you see:\r\n", ch );
            case ITEM_KEYRING:
                EXA_prog_trigger = FALSE;
                snprintf( buf, MSL, "in %s", arg );
                do_look( ch, buf );
                EXA_prog_trigger = TRUE;
                break;
        }
        if ( IS_OBJ_STAT( obj, ITEM_COVERING ) ) {
            EXA_prog_trigger = FALSE;
            snprintf( buf, MSL, "under %s", arg );
            do_look( ch, buf );
            EXA_prog_trigger = TRUE;
        }
        oprog_examine_trigger( ch, obj );
        if ( char_died( ch ) || obj_extracted( obj ) )
            return;

        check_for_trap( ch, obj, TRAP_EXAMINE );
    }
    return;
}

void do_exits( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    EXIT_DATA              *pexit;
    bool                    found,
                            closed = FALSE,
        locked = FALSE,
        DT = FALSE;
    bool                    fAuto;
    int                     spaces;

    set_char_color( AT_EXITS, ch );
    fAuto = !str_cmp( argument, "auto" );
    if ( !check_blind( ch ) )
        return;
    mudstrlcpy( buf, fAuto ? "&WExits:" : "&WObvious exits:\r\n", MSL );
    found = FALSE;
    for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
        if ( pexit->to_room && !IS_SET( pexit->exit_info, EX_SECRET )
             && ( !IS_SET( pexit->exit_info, EX_WINDOW ) || IS_SET( pexit->exit_info, EX_ISDOOR ) )
             && !IS_SET( pexit->exit_info, EX_HIDDEN ) && !IS_SET( pexit->exit_info, EX_DIG ) ) {
            found = TRUE;
            closed = IS_SET( pexit->exit_info, EX_CLOSED );
            locked = ( IS_SET( pexit->exit_info, EX_LOCKED )
                       || IS_SET( pexit->exit_info, EX_BOLTED ) );
            DT = ( ( IS_IMMORTAL( ch ) || IS_AFFECTED( ch, AFF_DETECTTRAPS ) )
                   && IS_SET( pexit->to_room->room_flags, ROOM_DEATH ) );
            if ( fAuto ) {
                mudstrlcat( buf, " ", MSL );
                mudstrlcat( buf, DT ? "&R***" : !closed ? "" : locked ? "&R[&W" : "[", MSL );

                // If exitflag AUTOFULL, display exit name by keyword. -Taon
                if ( IS_SET( pexit->exit_info, EX_AUTOFULL ) )
                    mudstrlcat( buf, DT ? strupper( pexit->keyword ) : pexit->keyword, MSL );
                else
                    mudstrlcat( buf, DT ? strupper( dir_name[pexit->vdir] ) : dir_name[pexit->vdir],
                                MSL );

                mudstrlcat( buf, DT ? "&R***&W" : !closed ? "" : locked ? "&R]&W" : "]", MSL );
            }
            else {
                spaces = 5 - strlen( dir_name[pexit->vdir] );
                if ( spaces < 0 )
                    spaces = 0;

                if ( !IS_SET( pexit->exit_info, EX_AUTOFULL ) )
                    snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%s" "%*s - %s \r\n", capitalize( dir_name[pexit->vdir] ), spaces,  /* number 
                                                                                                                                             * of 
                                                                                                                                             * spaces 
                                                                                                                                             */
                              "",
                              DT ? "&RYou sense an aura of imminent doom this way&W" :
                              locked ? "&R[&WClosed and Locked/Barred&R]&W" : closed ? "[Closed]" :
                              room_is_dark( pexit->to_room ) ? "Too dark to tell" : pexit->to_room->
                              name );
                else
                    snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%s" "%*s - %s\r\n", capitalize( pexit->keyword ), spaces,  /* number 
                                                                                                                                     * of 
                                                                                                                                     * spaces 
                                                                                                                                     */
                              "",
                              DT ? "&RYou sense an aura of imminent doom this way&W" :
                              locked ? "&R[&WClosed and Locked/Barred&R]&W" : closed ? "[Closed]" :
                              room_is_dark( pexit->to_room ) ? "Too dark to tell" : pexit->to_room->
                              name );
            }
        }
    }
    if ( !found )
        mudstrlcat( buf, fAuto ? " none.\r\n" : "None.\r\n", MSL );
    else if ( fAuto )
        mudstrlcat( buf, ".\r\n", MSL );
    send_to_char( buf, ch );
    return;
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing     -Thoric
 */
HELP_DATA              *get_help( CHAR_DATA *ch, char *argument )
{
    char                    argall[MIL];
    char                    argone[MIL];
    char                    argnew[MIL];
    HELP_DATA              *pHelp;
    int                     lev;

    if ( argument[0] == '\0' )
        argument = ( char * ) "help";

    if ( isdigit( argument[0] ) ) {
        lev = number_argument( argument, argnew );
        argument = argnew;
    }
    else
        lev = -2;
    /*
     * Tricky argument handling so 'help a b' doesn't match a.
     */
    argall[0] = '\0';
    while ( argument[0] != '\0' ) {
        argument = one_argument( argument, argone );
        if ( argall[0] != '\0' )
            mudstrlcat( argall, " ", MIL );
        mudstrlcat( argall, argone, MIL );
    }

    for ( pHelp = first_help; pHelp; pHelp = pHelp->next ) {
        if ( pHelp->level > get_trust( ch ) )
            continue;
        if ( lev != -2 && pHelp->level != lev )
            continue;

        if ( is_name( argall, pHelp->keyword ) )
            return pHelp;
    }

    return NULL;
}

/*
 * LAWS command
 */
void do_laws( CHAR_DATA *ch, char *argument )
{
    char                    buf[1024];

    if ( argument == NULL )
        do_help( ch, ( char * ) "laws" );
    else {
        snprintf( buf, 1024, "law %s", argument );
        do_help( ch, buf );
    }
}

void show_help( CHAR_DATA *ch, HELP_DATA *pHelp )
{
    const char             *outline =
        "&b-=-&B=-=&b-=-&B=-=&b-=-&B=-=&b-=-&B-=-&b=&B-=-&b=-&B=-=&b-=-&B=-=&b-=-&B=-=&b-=-&B=-=&b-=-&B=-=&b-=-&B=-=&b-=-&D\r\n";

    if ( !ch )
        return;
    if ( !pHelp ) {
        send_to_pager( "&wNo such help found.\r\n", ch );
        return;
    }

    set_pager_color( AT_HELP, ch );
    if ( pHelp->level >= 0 ) {
        send_to_pager( outline, ch );
        if ( VLD_STR( pHelp->keyword ) )
            send_to_pager( pHelp->keyword, ch );
        send_to_pager( "\r\n", ch );
    }
    /*
     * Strip leading '.' to allow initial blanks. 
     */
    send_to_pager( "\r\n", ch );
    if ( VLD_STR( pHelp->text ) ) {
        if ( pHelp->text[0] == '.' )
            send_to_pager( pHelp->text + 1, ch );
        else
            send_to_pager( pHelp->text, ch );
    }
    if ( IS_IMMORTAL( ch ) ) {
        pager_printf( ch, "\r\n&wLast Updated [&W%24.24s&w]&D\r\n",
                      ( char * ) ctime( &pHelp->modified_time ) );
        pager_printf( ch, "&wHelpfile Level [&W%d&w]&D\r\n", pHelp->level );
    }
    if ( pHelp->level >= 0 )
        send_to_pager( outline, ch );
    return;
}

/*
 * Now this is cleaner
 */
/* Updated do_help command provided by Remcon of The Lands of Pabulum 03/20/2004 */
void do_help( CHAR_DATA *ch, char *argument )
{
    HELP_DATA              *pHelp;
    char                   *keyword;
    char                    arg[MAX_INPUT_LENGTH];
    char                    oneword[MAX_STRING_LENGTH],
                            lastmatch[MAX_STRING_LENGTH];
    short                   matched = 0,
        checked = 0,
        totalmatched = 0,
        found = 0;
    bool                    uselevel = FALSE;
    int                     value = 0;

    set_pager_color( AT_NOTE, ch );

    if ( !argument || argument[0] == '\0' )
        argument = ( char * ) "help";
    if ( !( pHelp = get_help( ch, argument ) ) ) {
        pager_printf( ch, "No help on \'%s\' found.\r\n", argument );
        /*
         * Get an arg incase they do a number seperate 
         */
        one_argument( argument, arg );
        /*
         * See if arg is a number if so update argument 
         */
        if ( is_number( arg ) ) {
            argument = one_argument( argument, arg );
            if ( argument && argument[0] != '\0' ) {
                value = atoi( arg );
                uselevel = TRUE;
            }
            else                                       /* If no more argument put arg as
                                                        * * * * * argument */
                argument = arg;
        }
        if ( value > 0 )
            pager_printf( ch, "Checking for suggested helps that are level %d.\r\n", value );
        send_to_pager( "Suggested Help Files:\r\n", ch );
        strncpy( lastmatch, " ", MAX_STRING_LENGTH );
        for ( pHelp = first_help; pHelp; pHelp = pHelp->next ) {
            matched = 0;
            if ( !pHelp || !pHelp->keyword || pHelp->keyword[0] == '\0'
                 || pHelp->level > get_trust( ch ) )
                continue;
            /*
             * Check arg if its avaliable 
             */
            if ( uselevel && pHelp->level != value )
                continue;
            keyword = pHelp->keyword;
            while ( keyword && keyword[0] != '\0' ) {
                matched = 0;                           /* Set to 0 for each time we check 
                                                        * lol */
                keyword = one_argument( keyword, oneword );
                /*
                 * Lets check only up to 10 spots
                 */
                for ( checked = 0; checked <= 10; checked++ ) {
                    if ( !oneword[checked] || !argument[checked] )
                        break;
                    if ( LOWER( oneword[checked] ) == LOWER( argument[checked] ) )
                        matched++;
                }
                if ( ( matched > 1 && matched > ( checked / 2 ) )
                     || ( matched > 0 && checked < 2 ) ) {
                    pager_printf( ch, " %-20s ", oneword );
                    if ( ++found % 4 == 0 ) {
                        found = 0;
                        send_to_pager( "\r\n", ch );
                    }
                    strncpy( lastmatch, oneword, MAX_STRING_LENGTH );
                    totalmatched++;
                    break;
                }
            }
        }
        if ( found != 0 )
            send_to_pager( "\r\n", ch );
        if ( totalmatched == 0 ) {
            send_to_pager( "No suggested help files.\r\n", ch );
            return;
        }
        if ( totalmatched == 1 && lastmatch != NULL && lastmatch && lastmatch[0] != '\0'
             && str_cmp( lastmatch, argument ) ) {
            send_to_pager( "Opening only suggested helpfile.\r\n", ch );
            do_help( ch, lastmatch );
            return;
        }
        return;
    }
    /*
     * Make newbies do a help start. --Shaddai
     */

    if ( IS_IMMORTAL( ch ) )
        pager_printf( ch, "Help level: %d\r\n", pHelp->level );

    set_pager_color( AT_NOTE, ch );

    /*
     * Strip leading '.' to allow initial blanks.
     */
    if ( !pHelp->text )
        send_to_pager( "No text found.", ch );
    else if ( pHelp->text[0] == '.' )
        send_to_pager( pHelp->text + 1, ch );
    else
        send_to_pager( pHelp->text, ch );
    return;
}

/* Title : Help Check Plus v1.0
 * Author: Chris Coulter (aka Gabriel Androctus)
 * Email : krisco7@bigfoot.com
 * Mud   : Perils of Quiernin (perils.wolfpaw.net 6000)
 * Descr.: A ridiculously simple routine that runs through the command and skill tables
 *         checking for help entries of the same name. If not found, it outputs a line
 *         to the pager. Priceless tool for finding those pesky missing help entries.
 */
void do_helpcheck( CHAR_DATA *ch, char *argument )
{
    CMDTYPE                *command;
    HELP_DATA              *help;
    int                     hash,
                            sn;
    int                     total = 0;
    bool                    fSkills = FALSE;
    bool                    fCmds = FALSE;

    if ( argument[0] == '\0' ) {
        set_pager_color( AT_YELLOW, ch );
        send_to_pager( "Syntax: helpcheck [ skills | commands | all ]\r\n", ch );
        return;
    }

    /*
     * check arguments and set appropriate switches 
     */
    if ( !str_cmp( argument, "skills" ) )
        fSkills = TRUE;
    if ( !str_cmp( argument, "commands" ) )
        fCmds = TRUE;
    if ( !str_cmp( argument, "all" ) ) {
        fSkills = TRUE;
        fCmds = TRUE;
    }
    if ( fCmds ) {                                     /* run through command table */
        send_to_pager( "&CMissing Commands Helps\r\n\r\n", ch );
        for ( hash = 0; hash < 126; hash++ )
            for ( command = command_hash[hash]; command; command = command->next ) {
                /*
                 * No entry, or command is above person's level 
                 */
                if ( !( help = get_help( ch, command->name ) ) && command->level <= ch->level ) {
                    pager_printf_color( ch, "&cNot found: &C%s&w\r\n", command->name );
                    total++;
                }
                else
                    continue;
            }
    }
    if ( fSkills ) {                                   /* run through skill table */
        send_to_pager( "\r\n&CMissing Skill/Spell Helps\r\n\r\n", ch );
        for ( sn = 0; sn < top_sn; sn++ ) {
            if ( !( help = get_help( ch, skill_table[sn]->name ) ) ) {  /* no help entry */
                pager_printf_color( ch, "&gNot found: &G%s&w\r\n", skill_table[sn]->name );
                total++;
            }
            else
                continue;
        }
    }
    /*
     * tally up the total number of missing entries and finish up 
     */
    pager_printf_color( ch, "\r\n&Y%d missing entries found.&w\r\n", total );
    return;
}

extern char            *help_greeting;                 /* so we can edit the greeting
                                                        * online */

/* Help editor -Thoric */
void do_hedit( CHAR_DATA *ch, char *argument )
{
    HELP_DATA              *pHelp;
    struct timeval          now_time;

    if ( !ch->desc ) {
        send_to_char( "You have no descriptor.\r\n", ch );
        return;
    }
    switch ( ch->substate ) {
        default:
            break;
        case SUB_HELP_EDIT:
            if ( ( pHelp = ( HELP_DATA * ) ch->dest_buf ) == NULL ) {
                bug( "%s", "hedit: sub_help_edit: NULL ch->dest_buf" );
                stop_editing( ch );
                return;
            }
            if ( help_greeting == pHelp->text )
                help_greeting = NULL;
            if ( VLD_STR( pHelp->text ) )
                STRFREE( pHelp->text );
            pHelp->text = copy_buffer( ch );
            if ( !help_greeting )
                help_greeting = pHelp->text;
            stop_editing( ch );
            gettimeofday( &now_time, NULL );
            pHelp->modified_time = ( time_t ) now_time.tv_sec;
            return;
    }
    if ( ( pHelp = get_help( ch, argument ) ) == NULL ) {   /* new help */
        HELP_DATA              *tHelp;
        char                    argnew[MIL];
        int                     lev;
        bool                    new_help = TRUE;

        for ( tHelp = first_help; tHelp; tHelp = tHelp->next )
            if ( !str_cmp( argument, tHelp->keyword ) ) {
                pHelp = tHelp;
                new_help = FALSE;
                break;
            }
        if ( new_help ) {
            if ( isdigit( argument[0] ) ) {
                lev = number_argument( argument, argnew );
                argument = argnew;
            }
            else
                lev = get_trust( ch );
            CREATE( pHelp, HELP_DATA, 1 );

            pHelp->keyword = STRALLOC( strupper( argument ) );
            pHelp->level = lev;
            add_help( pHelp );
        }
    }
    ch->substate = SUB_HELP_EDIT;
    ch->dest_buf = pHelp;
    start_editing( ch, pHelp->text );
    editor_desc_printf( ch, "Help topic, keyword '%s', level %d.", pHelp->keyword, pHelp->level );
}

/* Stupid leading space muncher fix -Thoric */
char                   *help_fix( char *text )
{
    char                   *fixed;

    if ( !text )
        return ( char * ) "";
    fixed = strip_cr( text );
    if ( fixed[0] == ' ' )
        fixed[0] = '.';
    return fixed;
}

void do_hset( CHAR_DATA *ch, char *argument )
{
    HELP_DATA              *pHelp,
                           *pHelp_next;
    char                    arg1[MIL];
    char                    arg2[MIL];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: hset <field> [value] [help page]\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  level keyword reload remove save\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "reload" ) && ch->level >= ( MAX_LEVEL - 1 ) ) {
        log_string( "Unloading existing help files." );
        for ( pHelp = first_help; pHelp; pHelp = pHelp_next ) {
            pHelp_next = pHelp->next;
            UNLINK( pHelp, first_help, last_help, next, prev );
            free_help( pHelp );
        }
        top_help = 0;
        log_string( "Reloading help files." );
        load_area_file( last_area, "help.are" );
        send_to_char( "Help files reloaded.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "save" ) ) {
        FILE                   *fpout;
        struct timeval          now_time;

        log_string_plus( "Saving help.are...", LOG_NORMAL, LEVEL_IMMORTAL );
        remove( "area/help.are.bak" );
        rename( "area/help.are", "area/help.are.bak" );
        if ( ( fpout = FileOpen( "area/help.are", "w" ) ) == NULL ) {
            bug( "hset save: can't open area/help.are" );
            perror( "area/help.are" );
            return;
        }
        fprintf( fpout, "#HELPS\n\n" );
        for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
            fprintf( fpout, "%d %ld %s~\n%s~\n\n", pHelp->level, pHelp->modified_time,
                     VLD_STR( pHelp->keyword ) ? pHelp->keyword : "",
                     VLD_STR( pHelp->text ) ? help_fix( pHelp->text ) : "" );
        gettimeofday( &now_time, NULL );
        fprintf( fpout, "0 %ld $~\n\n\n#$\n", ( time_t ) now_time.tv_sec );
        FileClose( fpout );
        send_to_char( "Saved.\r\n", ch );
        return;
    }
    if ( str_cmp( arg1, "remove" ) )
        argument = one_argument( argument, arg2 );
    if ( ( pHelp = get_help( ch, argument ) ) == NULL ) {
        send_to_char( "Cannot find help on that subject.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "remove" ) ) {
        UNLINK( pHelp, first_help, last_help, next, prev );
        if ( VLD_STR( pHelp->text ) )
            STRFREE( pHelp->text );
        if ( VLD_STR( pHelp->keyword ) )
            STRFREE( pHelp->keyword );
        DISPOSE( pHelp );
        send_to_char( "Removed.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "level" ) ) {
        pHelp->level = atoi( arg2 );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "keyword" ) ) {
        if ( !VLD_STR( arg2 ) ) {
            send_to_char( "Set the keyword to what?\r\n", ch );
            return;
        }
        if ( VLD_STR( pHelp->keyword ) )
            STRFREE( pHelp->keyword );
        pHelp->keyword = STRALLOC( strupper( arg2 ) );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    do_hset( ch, ( char * ) "" );
}

void do_hl( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to use HLIST, spell it out.\r\n", ch );
    return;
}

/* Show help topics in a level range -Thoric
 * Idea suggested by Gorog
 * prefix keyword indexing added by Fireblade */
void do_hlist( CHAR_DATA *ch, char *argument )
{
    int                     min,
                            max,
                            value,
                            cnt,
                            level,
                            count,
                            col;
    char                    arg[MIL];
    HELP_DATA              *help;
    bool                    minfound,
                            maxfound;
    char                   *idx;

    if ( !ch || IS_NPC( ch ) )
        return;
    max = get_trust( ch );
    min = ( max >= LEVEL_IMMORTAL ? -1 : 0 );
    idx = NULL;
    minfound = FALSE;
    maxfound = FALSE;
    for ( argument = one_argument( argument, arg ); arg[0] != '\0';
          argument = one_argument( argument, arg ) ) {
        if ( !is_number( arg ) ) {
            if ( idx ) {
                set_char_color( AT_GREEN, ch );
                send_to_char( "You may only use a single keyword to index the list.\r\n", ch );
                return;
            }
            idx = STRALLOC( arg );
        }
        else {
            value = atoi( arg );

            if ( !minfound && value >= min && value < max ) {
                min = value;
                minfound = TRUE;
            }
            else if ( !maxfound && value > min && value <= max ) {
                max = value;
                maxfound = TRUE;
            }
            else if ( value < min || value > max ) {
                ch_printf( ch, "Valid levels are %d to %d.\r\n", ( max >= LEVEL_IMMORTAL ? -1 : 0 ),
                           get_trust( ch ) );
                return;
            }
            else if ( minfound && maxfound )
                break;
        }
    }
    if ( min > max ) {
        int                     temp = min;

        min = max;
        max = temp;
    }
    set_pager_color( AT_GREEN, ch );
    pager_printf( ch, "\r\nHelp Topics in level range %d to %d:", min, max );
    cnt = 0;
    col = 0;
    for ( level = -1; level <= MAX_LEVEL; level++ ) {
        count = 0;
        col = 0;
        for ( help = first_help; help; help = help->next ) {
            if ( help->level >= min && help->level <= max && help->level == level
                 && ( !idx || nifty_is_name_prefix( idx, help->keyword ) ) ) {
                count++;
                if ( count == 1 )
                    pager_printf( ch, "\r\n[Level %3d]\r\n", level );
                pager_printf( ch, "%-30.30s ", help->keyword );
                cnt++;
                col++;
                if ( col == 3 ) {
                    send_to_pager( "\r\n", ch );
                    col = 0;
                }
            }
        }
        if ( col != 0 )
            send_to_pager( "\r\n", ch );
    }
    if ( cnt != 0 )
        pager_printf( ch, "\r\n\r\n%d pages found.\r\n", cnt );
    else
        send_to_char( "\r\n\r\nNone found.\r\n", ch );
    if ( idx )
        STRFREE( idx );
    return;
}

/* Ported by Volk - 23/8/06 - Thanks to Samson
 * Derived directly from the i3who code, which is a hybrid mix of Smaug, RM, and Dale who. */
int new_who( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *person;
    char                    s1[16],
                            s2[16],
                            s3[16],
                            s4[16],
                            s5[16],
                            s6[16],
                            s7[16];
    char                    constate[16];
    char                    rank[200],
                            clan_name[MIL],
                            city_name[MIL],
                            invis_str[50],
                            council_name[MIL];
    char                    col[MSL];
    int                     pcount = 0,
        color = 0;
    bool                    skipconstate;
    bool                    first;
    bool                    sdeadly = FALSE;
    bool                    speaceful = FALSE;

    if ( argument && argument[0] != '\0' ) {
        if ( !str_cmp( argument, "deadly" ) )
            sdeadly = TRUE;
        else if ( !str_cmp( argument, "peaceful" ) )
            speaceful = TRUE;
    }
    snprintf( s1, 16, "%s", color_str( AT_WHO, ch ) );
    snprintf( s2, 16, "%s", color_str( AT_WHO2, ch ) );
    snprintf( s3, 16, "%s", color_str( AT_WHO3, ch ) );
    snprintf( s4, 16, "%s", color_str( AT_WHO4, ch ) );
    snprintf( s5, 16, "%s", color_str( AT_WHO5, ch ) );
    snprintf( s6, 16, "%s", color_str( AT_WHO6, ch ) );
    snprintf( s7, 16, "%s", color_str( AT_WHO7, ch ) );

    first = TRUE;
    for ( person = first_char; person; person = person->next ) {
        skipconstate = FALSE;

        if ( IS_NPC( person ) || IS_IMMORTAL( person ) )
            continue;

        if ( !IS_PUPPET( person ) && !person->desc )
            continue;

        if ( sdeadly && !IS_PKILL( person ) )
            continue;
        if ( speaceful && IS_PKILL( person ) )
            continue;

        if ( IS_IMMORTAL( person ) ) {
            if ( !can_see( ch, person ) )
                continue;
        }

        pcount++;

        if ( person->desc ) {
            switch ( person->desc->connected ) {
                default:
                    constate[0] = '\0';
                    break;

                case CON_NOTE_SUBJECT:
                case CON_NOTE_EXPIRE:
                case CON_NOTE_TO:
                case CON_EDITING:
                    snprintf( constate, sizeof( constate ), " %s[%sEditing%s]", s5, s2, s5 );
                    break;

                case CON_CONFIRM_NEW_NAME:
                case CON_GET_NEW_PASSWORD:
                case CON_CONFIRM_NEW_PASSWORD:
                case CON_GET_NEW_SEX:
                case CON_GET_NEW_RACE:
                case CON_GET_NEW_CLASS:
                case CON_GET_FIRST_CHOICE:
                case CON_GET_SECOND_CHOICE:
                case CON_GET_SECOND_CLASS:
                case CON_GET_THIRD_CLASS:
                case CON_PRESS_ENTER:
                case CON_ROLL_STATS:
                case CON_GET_NAME:
                case CON_GET_OLD_PASSWORD:
                    skipconstate = TRUE;
                    break;
            }
            if ( skipconstate )
                continue;
        }

        if ( IS_PLR_FLAG( person, PLR_AFK ) )
            snprintf( constate, sizeof( constate ), " %s[%sAFK%s]", s5, s2, s5 );
        else if ( IS_PLR_FLAG( person, PLR_RP ) )
            snprintf( constate, sizeof( constate ), " %s[%sRP%s]", s5, s2, s5 );
        else
            constate[0] = '\0';

        if ( first ) {
            if ( !IS_BLIND( ch ) )
                pager_printf( ch,
                              "\r\n%s--------------------------------=[ %sPlayers %s]=---------------------------------\r\n\r\n",
                              s5, s6, s5 );
            else
                pager_printf( ch, "\r\nPlayers\r\n\r\n" );
            first = FALSE;
        }

        if ( ch->level > person->level )
            color = ch->level - person->level;
        if ( ch->level < person->level )
            color = ch->level - person->level;
        else
            color = 0;

        snprintf( col, MSL, "%s", color > 5 ? ( color > 10 ? "&R" : "&Y" ) : "&G" );

        if ( IS_THIRDCLASS( person ) ) {
            if ( IS_BLIND( ch ) )
                snprintf( rank, 200, "%d tri class %s %s %s.", person->level,
                          class_table[person->Class]->who_name,
                          class_table[person->secondclass]->who_name,
                          class_table[person->thirdclass]->who_name );
            else
                snprintf( rank, 200, "%s[%s%-3d%s][%s%-3.3s%s/%s%-3.3s%s/%s%-3.3s%s]",
                          s5, col, person->level, s5, s6, class_table[person->Class]->who_name, s5,
                          s6, class_table[person->secondclass]->who_name, s5, s6,
                          class_table[person->thirdclass]->who_name, s5 );
        }
        else if ( IS_SECONDCLASS( person ) ) {
            if ( IS_BLIND( ch ) )
                snprintf( rank, 200, "%d dual class %s %s.", person->level,
                          class_table[person->Class]->who_name,
                          class_table[person->secondclass]->who_name );
            else
                snprintf( rank, 200, "%s[%s%-3d%s][%s%-5.5s%s/%s%-5.5s%s]", s5, col, person->level,
                          s5, s6, class_table[person->Class]->who_name, s5, s6,
                          class_table[person->secondclass]->who_name, s5 );
        }
        else {
            if ( IS_BLIND( ch ) )
                snprintf( rank, 200, "%d class %s.", person->level,
                          class_table[person->Class]->who_name );
            else
                snprintf( rank, 200, "%s[%s%-3d%s][%s%-11.11s%s]", s5, col, person->level, s5, s6,
                          class_table[person->Class]->who_name, s5 );
        }

        send_to_pager( rank, ch );

        if ( IS_CITY( person ) ) {
            CITY_DATA              *city = person->pcdata->city;

            mudstrlcpy( city_name, " &C[&W", MIL );
            if ( VLD_STR( person->pcdata->city->duke )
                 && !str_cmp( person->name, person->pcdata->city->duke ) ) {
                if ( person->sex == SEX_FEMALE )
                    mudstrlcat( city_name, "Duchess of ", MIL );
                else
                    mudstrlcat( city_name, "Duke of ", MIL );
            }
            else if ( VLD_STR( person->pcdata->city->baron )
                      && !str_cmp( person->name, person->pcdata->city->baron ) ) {
                if ( person->sex == SEX_FEMALE )
                    mudstrlcat( city_name, "Baroness of ", MIL );
                else
                    mudstrlcat( city_name, "Baron of ", MIL );
            }
            else if ( VLD_STR( person->pcdata->city->captain )
                      && !str_cmp( person->name, person->pcdata->city->captain ) )
                mudstrlcat( city_name, "Captain of ", MIL );
            else if ( VLD_STR( person->pcdata->city->sheriff )
                      && !str_cmp( person->name, person->pcdata->city->sheriff ) )
                mudstrlcat( city_name, "Sheriff of ", MIL );
            else if ( VLD_STR( person->pcdata->city->knight )
                      && !str_cmp( person->name, person->pcdata->city->knight ) )
                mudstrlcat( city_name, "Knight of ", MIL );
            mudstrlcat( city_name, person->pcdata->city_name, MIL );
            mudstrlcat( city_name, "&C]&G", MIL );
        }
        else
            city_name[0] = '\0';

        if ( IS_CLANNED( person ) ) {
            CLAN_DATA              *clan = person->pcdata->clan;

            mudstrlcpy( clan_name, " &C[&W", MIL );

            if ( VLD_STR( person->pcdata->clan->chieftain )
                 && !str_cmp( person->name, person->pcdata->clan->chieftain ) )
                mudstrlcat( clan_name, "Chieftain of ", MIL );
            else if ( VLD_STR( person->pcdata->clan->warmaster )
                      && !str_cmp( person->name, person->pcdata->clan->warmaster ) ) {
                if ( !str_cmp( clan->name, "Halcyon" ) )
                    mudstrlcat( clan_name, "Ambassador of ", MIL );
                else
                    mudstrlcat( clan_name, "War Master of ", MIL );
            }
            mudstrlcat( clan_name, person->pcdata->clan_name, MIL );
            mudstrlcat( clan_name, "&C]&G", MIL );
        }
        else
            clan_name[0] = '\0';

        if ( !IS_BLIND( ch ) )
            pager_printf( ch, " %s%s&z%s%s%s%s\r\n", IS_PKILL( person ) ? "&R" : "&C", person->name,
                          person->pcdata->title, clan_name, city_name, constate );
        else
            pager_printf( ch, " %s%s&z%s%s%s%s\r\n", IS_PKILL( person ) ? "Deadly " : "Peaceful ",
                          person->name, person->pcdata->title, clan_name, city_name, constate );
    }

    first = TRUE;
    for ( d = first_descriptor; d; d = d->next ) {
        skipconstate = FALSE;

        person = d->original ? d->original : d->character;

        if ( person ) {
            if ( person->level < LEVEL_IMMORTAL )
                continue;

            if ( ch ) {
                if ( is_ignoring( person, ch ) )
                    continue;
            }
            else {
                if ( IS_PLR_FLAG( person, PLR_WIZINVIS ) )
                    continue;
            }

            if ( IS_IMMORTAL( person ) ) {
                if ( !can_see( ch, person ) )
                    continue;
            }

            switch ( d->connected ) {
                default:
                    constate[0] = '\0';
                    break;

                case CON_NOTE_SUBJECT:
                case CON_NOTE_EXPIRE:
                case CON_NOTE_TO:
                case CON_EDITING:
                    snprintf( constate, sizeof( constate ), " %s[%sEditing%s]", s5, s2, s5 );
                    break;

                case CON_CONFIRM_NEW_NAME:
                case CON_GET_NEW_PASSWORD:
                case CON_CONFIRM_NEW_PASSWORD:
                case CON_GET_NEW_SEX:
                case CON_GET_NEW_RACE:
                case CON_GET_NEW_CLASS:
                case CON_GET_FIRST_CHOICE:
                case CON_GET_SECOND_CHOICE:
                case CON_GET_SECOND_CLASS:
                case CON_GET_THIRD_CLASS:
                case CON_PRESS_ENTER:
                case CON_ROLL_STATS:
                case CON_GET_NAME:
                case CON_GET_OLD_PASSWORD:
                    skipconstate = TRUE;
                    break;
            }

            if ( skipconstate )
                continue;

            if ( !constate[0] && IS_PLR_FLAG( person, PLR_AFK ) )
                snprintf( constate, sizeof( constate ), " %s[%sAFK%s]", s5, s2, s5 );
            if ( !constate[0] && IS_PLR_FLAG( person, PLR_RP ) )
                snprintf( constate, sizeof( constate ), " %s[%sRP%s]", s5, s2, s5 );

            if ( first == TRUE ) {
                if ( IS_BLIND( ch ) )
                    pager_printf( ch, "\r\n&d%sSTAFF  %s\r\n\r\n", s1, s6, s1 );
                else
                    pager_printf( ch,
                                  "\r\n&d%s--------------------------------=[  %sSTAFF  %s]=---------------------------------\r\n\r\n",
                                  s1, s6, s1 );
                first = FALSE;
            }
            pcount++;
            if ( !person->pcdata->rank || !*person->pcdata->rank ) {
                switch ( person->level ) {
                    default:
                        mudstrlcpy( rank, "&r6 DRAGONS", MIL );
                        break;
                    case MAX_LEVEL:
                        mudstrlcpy( rank, "&rGENERAL", MIL );
                        break;
                    case LEVEL_AJ_COLONEL:
                        mudstrlcpy( rank, "&rCOLONEL", MIL );
                        break;
                    case LEVEL_AJ_MAJOR:
                        mudstrlcpy( rank, "&rMAJOR", MIL );
                        break;
                    case LEVEL_AJ_CAPTAIN:
                        mudstrlcpy( rank, "&rCAPTAIN", MIL );
                        break;
                    case LEVEL_AJ_LT:
                        mudstrlcpy( rank, "&rLIEUTENANT", MIL );
                        break;
                    case LEVEL_AJ_SGT:
                        mudstrlcpy( rank, "&rSERGEANT", MIL );
                        break;
                    case LEVEL_AJ_CPL:
                        mudstrlcpy( rank, "&rCORPORAL", MIL );
                        break;
                    case LEVEL_IMMORTAL:
                        mudstrlcpy( rank, "&rPRIVATE", MIL );
                        break;
                }

                if ( IS_GUEST( person ) )
                    mudstrlcpy( rank, "&rGUEST", MIL );
                if ( IS_RETIRED( person ) )
                    mudstrlcpy( rank, "&rRETIRED", MIL );
                if ( IS_TEMP( person ) )
                    mudstrlcpy( rank, "&rTEMP", MIL );

                pager_printf( ch, "%-20.20s", rank );
            }
            else {
                int                     colorstr = color_strlen( person->pcdata->rank );
                int                     normstr = strlen( person->pcdata->rank );

                if ( colorstr == normstr && normstr <= 18 ) {
                    snprintf( rank, 200, "&D%s", person->pcdata->rank );
                    pager_printf( ch, "%-20.20s", rank );
                }
                else {
                    int                     diff = ( normstr - colorstr );
                    int                     count = colorstr;

                    snprintf( rank, 200, "&D%s", person->pcdata->rank );

                    /*
                     * Since it takes 2 things to make a color we need to add a space for every 2
                     * in diff 
                     */
                    send_to_pager( rank, ch );
                    while ( count < 18 && diff > 2 ) {
                        send_to_pager( " ", ch );
                        diff -= 2;
                        count++;
                    }

                    /*
                     * Ok we need to add in spaces to make it up to 20 if needed 
                     */
                    while ( count < 18 ) {
                        count++;
                        send_to_pager( " ", ch );
                    }
                }
            }
            /*
             * Cleaner clans - Volk 
             */

            if ( IS_PLR_FLAG( person, PLR_WIZINVIS ) )
                snprintf( invis_str, 50, " (%d)", person->pcdata->wizinvis );
            else
                invis_str[0] = '\0';

            const char             *extra_title;

            if ( VLD_STR( sysdata.clan_overseer )
                 && !str_cmp( person->name, sysdata.clan_overseer ) )
                extra_title = " &W[&POverseer of Clans&W]&D";
            else if ( VLD_STR( sysdata.clan_advisor )
                      && !str_cmp( person->name, sysdata.clan_advisor ) )
                extra_title = " &W[&PAdvisor to Clans&W]&D";
            else
                extra_title = "";

            if ( person->pcdata->council ) {
                mudstrlcpy( council_name, " &C[&W", MIL );
                if ( person->pcdata->council->head2 == NULL ) {
                    if ( VLD_STR( person->pcdata->council->head )
                         && !str_cmp( person->name, person->pcdata->council->head ) )
                        mudstrlcat( council_name, "Head of ", MIL );
                }
                else {
                    if ( ( VLD_STR( person->pcdata->council->head )
                           && !str_cmp( person->name, person->pcdata->council->head ) )
                         || ( VLD_STR( person->pcdata->council->head2 )
                              && !str_cmp( person->name, person->pcdata->council->head2 ) ) )
                        mudstrlcat( council_name, "Co-Head of ", MIL );
                }
                mudstrlcat( council_name, person->pcdata->council_name, MIL );
                mudstrlcat( council_name, "&C]&G", MIL );
            }
            else
                council_name[0] = '\0';

            const char             *title;

            if ( person->pcdata->afkbuf )
                title = person->pcdata->afkbuf;
            else
                title = person->pcdata->title;

            pager_printf( ch, "%s &W%s&z%s%s%s%s\r\n", invis_str, person->name, title, extra_title,
                          council_name, constate );
        }
    }

    return pcount;
}

//Code slightly modified to better support screen readers. -Taon
void do_who( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            outbuf[MSL];
    char                    s1[16],
                            s4[16],
                            s5[16],
                            s6[16];
    int                     amount = 0,
        xx = 0,
        pcount = 0;

    if ( !ch )
        return;

    snprintf( s1, 16, "%s", color_str( AT_WHO, ch ) );
    snprintf( s6, 16, "%s", color_str( AT_WHO6, ch ) );
    snprintf( s4, 16, "%s", color_str( AT_WHO4, ch ) );
    snprintf( s5, 16, "%s", color_str( AT_WHO5, ch ) );

    outbuf[0] = '\0';

    send_to_char( "\r\n", ch );

    if ( IS_BLIND( ch ) )
        snprintf( buf, MSL, "%s who list follows:", sysdata.mud_name );
    else
        snprintf( buf, MSL, "%s-=[ %s%s %s]=-", s5, s6, sysdata.mud_name, s5 );

    amount = 60;                                       /* Determine amount to put in
                                                        * front of line */

    if ( amount < 1 )
        amount = 1;

    amount = amount / 2;

    for ( xx = 0; xx < amount; xx++ )
        mudstrlcat( outbuf, " ", MSL );

    mudstrlcat( outbuf, buf, MSL );
    pager_printf( ch, "%s\r\n", outbuf );

    pcount = new_who( ch, argument );

    /*
     * People reporting problems with pcount vs maxplayers .. 
     */
    if ( pcount > sysdata.maxplayers ) {
        sysdata.maxplayers = pcount;

        if ( sysdata.maxplayers > sysdata.alltimemax ) {
            if ( sysdata.time_of_max )
                STRFREE( sysdata.time_of_max );
            snprintf( buf, MSL, "%24.24s", ctime( &current_time ) );
            sysdata.time_of_max = STRALLOC( buf );
            sysdata.alltimemax = sysdata.maxplayers;
            snprintf( buf, MSL, "Broke all-time maximum player record: %d", sysdata.alltimemax );
            announce( buf );
            save_sysdata( sysdata );
        }
    }

    /*
     * New change to give more experience based on players online 
     */
    int                     dcount;
    int                     gmb;

    if ( !num_players_online )
        get_curr_players(  );
    dcount = num_players_online;
    if ( dcount > 40 )
        dcount = 40;

    send_to_pager( "\r\n\r\n", ch );
    if ( dcount ) {
        gmb = ( dcount * sysdata.gmb );
        pager_printf( ch, "%s                         [GMB: %s%d%%%s see help GMB ]\r\n", s5, s4,
                      gmb, s5 );
    }
    if ( !IS_BLIND( ch ) ) {
        pager_printf( ch,
                      "%s [Homepage: %s%s%s]  [Current Players: %s%s%s]  [%s%d %sMax Since Reboot]&w\r\n",
                      s5, s6, sysdata.http, s5, s6, num_punct( pcount ), s5, s6, sysdata.maxplayers,
                      s5 );
        pager_printf( ch,
                      "%s       [%s Please vote for us daily at:%s http://6dragons.org/bin/vote.html %s]\r\n",
                      s5, s4, s6, s5 );
        pager_printf( ch,
                      "          [%s The World map is viewable at:%s http://6dragons.org/map.gif %s]\r\n ",
                      s4, s6, s5 );
    }
}

void do_compare( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj1;
    OBJ_DATA               *obj2;
    int                     value1;
    int                     value2;
    const char             *msg;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Compare what to what?\r\n", ch );
        return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "You do not have that item.\r\n", ch );
        return;
    }

    if ( arg2[0] == '\0' ) {
        for ( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content ) {
            if ( obj2->wear_loc != WEAR_NONE && can_see_obj( ch, obj2 )
                 && obj1->item_type == obj2->item_type
                 && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE ) != 0 )
                break;
        }

        if ( !obj2 ) {
            send_to_char( "You aren't wearing anything comparable.\r\n", ch );
            return;
        }
    }
    else {
        if ( ( obj2 = get_obj_carry( ch, arg2 ) ) == NULL ) {
            send_to_char( "You do not have that item.\r\n", ch );
            return;
        }
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if ( obj1 == obj2 ) {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type ) {
        msg = "You can't compare $p and $P.";
    }
    else {
        switch ( obj1->item_type ) {
            default:
                msg = "You can't compare $p and $P.";
                break;

            case ITEM_ARMOR:
                value1 = obj1->value[0];
                value2 = obj2->value[0];
                break;

            case ITEM_WEAPON:
                value1 = obj1->value[1] + obj1->value[2];
                value2 = obj2->value[1] + obj2->value[2];
                break;
        }
    }

    if ( !msg ) {
        if ( value1 == value2 )
            msg = "$p and $P look about the same.";
        else if ( value1 > value2 )
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
    return;
}

void do_where( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    DESCRIPTOR_DATA        *d;
    bool                    found;

    one_argument( argument, arg );

    if ( arg[0] != '\0' && ( victim = get_char_world( ch, arg ) ) && !IS_NPC( victim )
         && IS_SET( victim->pcdata->flags, PCFLAG_DND ) && get_trust( ch ) < get_trust( victim ) ) {
        act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
        return;
    }

    set_pager_color( AT_PERSON, ch );
    if ( arg[0] == '\0' ) {
        pager_printf( ch, "\r\nPlayers near you in %s:\r\n", ch->in_room->area->name );
        found = FALSE;
        for ( d = first_descriptor; d; d = d->next ) {
            if ( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
                 && ( victim = d->character ) != NULL && !IS_NPC( victim )
                 && victim->in_room && victim->in_room->area == ch->in_room->area
                 && can_see( ch, victim ) && ( get_trust( ch ) >= get_trust( victim )
                                               || !IS_SET( victim->pcdata->flags, PCFLAG_DND ) ) ) {
                found = TRUE;
                pager_printf_color( ch, "&P%-13s  ", victim->name );
                if ( IS_IMMORTAL( victim ) ) {
                    if ( !IS_NPC( victim ) )
                        send_to_pager_color( "&Y(&OSTAFF&Y)&D\t", ch );
                    else
                        send_to_pager_color( "&Y(&OImmortal&Y)&D\t", ch );
                }
                else
                    send_to_pager( "\t\t\t", ch );
                if ( IN_WILDERNESS( victim ) || IN_RIFT( victim ) )
                    gen_wilderness_name( victim );
                else
                    pager_printf_color( ch, "&P%s", victim->in_room->name );
                send_to_pager( "\r\n", ch );
            }
        }
        if ( !found )
            send_to_char( "None\r\n", ch );
    }
    else {
        found = FALSE;
        for ( victim = first_char; victim; victim = victim->next ) {
            if ( victim->in_room && victim->in_room->area == ch->in_room->area
                 && !IS_AFFECTED( victim, AFF_HIDE ) && !IS_AFFECTED( victim, AFF_SNEAK )
                 && can_see( ch, victim ) && is_name( arg, victim->name ) ) {
                found = TRUE;
                pager_printf( ch, "%-28s", PERS( victim, ch ) );
                if ( IN_WILDERNESS( victim ) || IN_RIFT( victim ) )
                    gen_wilderness_name( victim );
                else
                    pager_printf( ch, "%s", victim->in_room->name );
                send_to_pager( "\r\n", ch );
                break;
            }
        }
        if ( !found )
            act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }
}

void do_consider( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    const char             *msg = " ";
    int                     diff;

    if ( !ch )
        return;
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Consider killing whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        send_to_char( "They're not here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You decide you're pretty sure you could take yourself in a fight.\r\n", ch );
        return;
    }

    act( AT_CYAN, "You look $N over appraisingly, and think to yourself....\r\n", ch, NULL, victim,
         TO_CHAR );

    diff = ( ch->level - victim->level );
    if ( diff >= 10 )
        msg = "I could be a teacher for $N!";
    else if ( diff >= 5 )
        msg = "I am far more experienced than $N.";
    else if ( diff >= 1 )
        msg = "I am more experienced than $N.";
    else if ( diff == 0 )
        msg = "Hrm, we are about equally experienced.";
    else if ( diff <= -10 )
        msg = "$N would make a great teacher for me!";
    else if ( diff <= -5 )
        msg = "$N is far more experienced than me.";
    else if ( diff <= -1 )
        msg = "$N is more experienced than me.";
    act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

// Updated for extreme players, shouldn't need to include experience of 
// above...basically doubled/halved the values as extreme mode only 
// doubles difficulty - Aurin 12/5/2010
    if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
        diff = ( int ) ( GET_DAMROLL( ch ) - GET_DAMROLL( victim ) );
        if ( diff >= 20 )
            msg = "I can by far out damage $N!";
        else if ( diff >= 10 )
            msg = "I can do more damage than $N.";
        else if ( diff >= 5 )
            msg = "I probably can do more damage than $N.";
        else if ( diff == 0 )
            msg = "Hrm, looks like we would do about the same damage to each other.";
        else if ( diff <= -20 )
            msg = "$N is able to do a lot more damage than me!";
        else if ( diff <= -10 )
            msg = "$N is able to do more damage than me.";
        else if ( diff <= -5 )
            msg = "$N is able to do a little more damage than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

        diff = ( int ) GET_HITROLL( ch ) - GET_HITROLL( victim );
        if ( diff >= 20 )
            msg = "I am able to hit a lot more accurately than $N.";
        else if ( diff >= 10 )
            msg = "I am able to hit more accurately than $N.";
        else if ( diff >= 5 )
            msg = "I am able to hit a little more accurately than $N.";
        else if ( diff == 0 )
            msg = "I am able to hit as accurately as $N.";
        else if ( diff <= -20 )
            msg = "$N is able to hit a lot more accurately than me.";
        else if ( diff <= -10 )
            msg = "$N is able to hit more accurately than me.";
        else if ( diff <= -5 )
            msg = "$N is able to hit a little more accurately than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

// No need to change AC values, as extreme mode doesn't effect this.
        diff = ( int ) GET_AC( ch ) - GET_AC( victim );
        if ( diff >= 500 )
            msg = "I am a lot better armored than $N.";
        else if ( diff >= 250 )
            msg = "I am better armored than $N.";
        else if ( diff >= 1 )
            msg = "I am a little better armored than $N.";
        else if ( diff == 0 )
            msg = "I am as armored as $N.";
        else if ( diff <= -500 )
            msg = "$N is a lot better armored than me.";
        else if ( diff <= -250 )
            msg = "$N is better armored than me.";
        else if ( diff <= -1 )
            msg = "$N is a little better armored than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

        diff = ( int ) ( ( ch->hit - victim->hit ) );
        if ( diff >= 400 )
            msg = "I could take anything $N has to give me!";
        else if ( diff >= 200 )
            msg = "I could take more damage than $N.";
        else if ( diff >= 10 )
            msg = "I probably could take more damage than $N.";
        else if ( diff == 0 )
            msg = "Hrm, looks like we would be able to take about the same amount of damage.";
        else if ( diff <= -400 )
            msg = "$N can take a lot more damage than me!";
        else if ( diff <= -200 )
            msg = "$N can take more damage than me.";
        else if ( diff <= -10 )
            msg = "$N can take a little more damage than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );
    }
    else {
// The below are the original formula's.
        diff = ( int ) ( GET_DAMROLL( ch ) - GET_DAMROLL( victim ) );
        if ( diff >= 10 )
            msg = "I can by far out damage $N!";
        else if ( diff >= 5 )
            msg = "I can do more damage than $N.";
        else if ( diff >= 1 )
            msg = "I probably can do more damage than $N.";
        else if ( diff == 0 )
            msg = "Hrm, looks like we would do about the same damage to each other.";
        else if ( diff <= -10 )
            msg = "$N is able to do a lot more damage than me!";
        else if ( diff <= -5 )
            msg = "$N is able to do more damage than me.";
        else if ( diff <= -1 )
            msg = "$N is able to do a little more damage than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

        diff = ( int ) GET_HITROLL( ch ) - GET_HITROLL( victim );
        if ( diff >= 10 )
            msg = "I am able to hit a lot more accurately than $N.";
        else if ( diff >= 5 )
            msg = "I am able to hit more accurately than $N.";
        else if ( diff >= 1 )
            msg = "I am able to hit a little more accurately than $N.";
        else if ( diff == 0 )
            msg = "I am able to hit as accurately as $N.";
        else if ( diff <= -10 )
            msg = "$N is able to hit a lot more accurately than me.";
        else if ( diff <= -5 )
            msg = "$N is able to hit more often accurately me.";
        else if ( diff <= -1 )
            msg = "$N is able to hit a little more accurately than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

        diff = ( int ) GET_AC( ch ) - GET_AC( victim );
        if ( diff >= 500 )
            msg = "I am a lot better armored than $N.";
        else if ( diff >= 250 )
            msg = "I am better armored than $N.";
        else if ( diff >= 1 )
            msg = "I am a little better armored than $N.";
        else if ( diff == 0 )
            msg = "I am as armored as $N.";
        else if ( diff <= -500 )
            msg = "$N is a lot better armored than me.";
        else if ( diff <= -250 )
            msg = "$N is better armored than me.";
        else if ( diff <= -1 )
            msg = "$N is a little better armored than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );

        diff = ( int ) ( ( ch->hit - victim->hit ) );
        if ( diff >= 200 )
            msg = "I could take anything $N has to give me!";
        else if ( diff >= 100 )
            msg = "I can take more damage then $N.";
        else if ( diff >= 1 )
            msg = "I probably could take more damage then $N.";
        else if ( diff == 0 )
            msg = "Hrm, looks like we would be able to take about the same amount of damage.";
        else if ( diff <= -200 )
            msg = "$N can take a lot more damage than me!";
        else if ( diff <= -100 )
            msg = "$N can take more damage than me.";
        else if ( diff <= -1 )
            msg = "$N can take a little more damage than me.";
        act( AT_LBLUE, msg, ch, NULL, victim, TO_CHAR );
    }
}

int find_skill_class( CHAR_DATA *ch, int sn )
{
    int                     Class = -2;

    if ( skill_table[sn]->type == SKILL_RACIAL )
        return -1;

    if ( skill_table[sn]->skill_level[ch->Class] < 101 )
        Class = 1;

    if ( IS_SECONDCLASS( ch ) && skill_table[sn]->skill_level[ch->secondclass] < 101 )
        Class = 2;

    if ( IS_THIRDCLASS( ch ) && skill_table[sn]->skill_level[ch->thirdclass] < 101 )
        Class = 3;

    if ( Class == 1 )
        return ch->Class;
    if ( Class == 2 )
        return ch->secondclass;
    if ( Class == 3 )
        return ch->thirdclass;

    return Class;
}

/*
 * Place any skill types you don't want them to be able to practice
 * normally in this list.  Separate each with a space.
 * (Uses an is_name check). -- Altrag
 */
#define CANT_PRAC "Tongue"

void do_practice( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            arg[MSL];
    int                     sn,
                            tshow = SKILL_UNKNOWN;
    bool                    showdshow = FALSE;
    short                   highspell,
                            highskill,
                            usehigh;

    if ( IS_NPC( ch ) )
        return;

    one_argument( argument, arg );

    if ( VLD_STR( arg ) )
        tshow = get_skill( arg );

    if ( VLD_STR( arg ) && !str_cmp( arg, "dshow" ) ) {
        argument = one_argument( argument, arg );
        showdshow = TRUE;

        if ( VLD_STR( argument ) ) {
            sn = skill_lookup( argument );

            if ( ( sn == -1 ) || !CAN_LEARN( ch, sn, TRUE ) ) {
                send_to_char( "You have no control on seeing something you don't know.\r\n", ch );
                return;
            }
            ch->pcdata->dshowlearned[sn] = !ch->pcdata->dshowlearned[sn];
            ch_printf( ch, "You will %s see that on the practice list display.\r\n",
                       ch->pcdata->dshowlearned[sn] == FALSE ? "now" : "no longer" );
            return;
        }
    }

    if ( tshow != SKILL_UNKNOWN || !VLD_STR( argument ) || showdshow ) {
        int                     col = 0;
        short                   lasttype = SKILL_UNKNOWN,
            cnt = 1;

        set_pager_color( AT_CYAN, ch );
        for ( sn = 0; sn < top_sn; sn++ ) {
            if ( !skill_table[sn]->name )
                break;

            if ( tshow != SKILL_UNKNOWN && skill_table[sn]->type != tshow )
                continue;

            if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_tan || sn == gsn_bake
                 || sn == gsn_gather || sn == gsn_mix || sn == gsn_hunt )
                continue;

            if ( !CAN_LEARN( ch, sn, TRUE ) )
                continue;

            if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
                continue;

            if ( !showdshow && ch->pcdata->dshowlearned[sn] == TRUE )   /* Don't show if
                                                                         * * dshowlearned 
                                                                         * is TRUE */
                continue;

            if ( showdshow && ch->pcdata->dshowlearned[sn] == FALSE )   /* Only show if * 
                                                                         * dshowlearned
                                                                         * is FALSE */
                continue;

            if ( skill_table[sn]->type != lasttype ) {
                int                     count,
                                        type_length;
                char                    space_buf[MSL];

                if ( !cnt )
                    send_to_pager( "                                   (none)\r\n", ch );
                else if ( col % 3 != 0 )
                    send_to_pager( "\r\n", ch );

                space_buf[0] = '\0';
                type_length =
                    ( ( 79 - ( strlen( skill_tname[skill_table[sn]->type] ) + 11 ) ) / 2 );
                for ( count = 0; count < type_length; count++ ) {
                    mudstrlcat( space_buf, " ", MSL );
                }
                set_pager_color( AT_CYAN, ch );
                pager_printf_color( ch, "%s&C_.:[", space_buf );
                set_pager_color( AT_CYAN, ch );
                pager_printf_color( ch, " &R%s&D ", skill_tname[skill_table[sn]->type] );
                set_pager_color( AT_CYAN, ch );
                send_to_pager_color( "&C]:._&D\r\n", ch );
                col = cnt = 0;
            }
            lasttype = skill_table[sn]->type;

            ++cnt;
            set_pager_color( AT_CYAN, ch );
            pager_printf( ch, "&c%20.20s&D", skill_table[sn]->name );
            if ( ch->pcdata->learned[sn] >= get_maxadept( ch, sn, TRUE ) )
                send_to_pager( "&W", ch );
            else if ( ch->pcdata->learned[sn] > 0 )
                send_to_pager( "&w", ch );
            else
                send_to_pager( "&z", ch );
            pager_printf( ch, " %3d%%%s&D ", ch->pcdata->learned[sn],
                          ( ch->pcdata->learned[sn] <= 0
                            && VLD_STR( skill_table[sn]->teachers ) ) ? "&R*" : " " );
            if ( ++col % 3 == 0 )
                send_to_pager( "\r\n", ch );
        }
        if ( col % 3 != 0 )
            send_to_pager( "\r\n", ch );
        if ( ch->pcdata->tradeclass > 1 ) {
            set_pager_color( AT_CYAN, ch );
            pager_printf_color( ch, "\t\t           &C_.:[" );
            set_pager_color( AT_CYAN, ch );
            pager_printf_color( ch, "  &RTrade Skills&D " );
            set_pager_color( AT_CYAN, ch );
            send_to_pager_color( "&C]:._&D\r\n", ch );
            col = cnt = 0;
// Updated to remove tradeskill skills from prac list if dshowlearned == 
// TRUE, or from the prac dshow list if dshowloearned == FALSE - Aurin
            if ( ch->pcdata->tradeclass == 20 ) {
                sn = gsn_mine;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
                sn = gsn_forge;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
            }
            if ( ch->pcdata->tradeclass == 21 ) {
                sn = gsn_gather;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
                sn = gsn_bake;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
                sn = gsn_mix;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
            }
            if ( ch->pcdata->tradeclass == 22 ) {
                sn = gsn_hunt;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% ", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
                sn = gsn_tan;
                if ( ( showdshow && ch->pcdata->dshowlearned[sn] == TRUE )
                     || ( !showdshow && ch->pcdata->dshowlearned[sn] == FALSE ) )
                    pager_printf( ch, "&c%20s &W%3d%% \r\n", skill_table[sn]->name,
                                  ch->pcdata->learned[sn] );
            }
            if ( col % 3 == 0 )
                send_to_pager( "\r\n", ch );
        }

        set_pager_color( AT_CYAN, ch );
        pager_printf( ch, "&cYou have &C%s&c practice sessions left.\r\n",
                      ch->practice > 0 ? num_punct( ch->practice ) : "no" );
    }
    else {
        CHAR_DATA              *mob;

        if ( !IS_AWAKE( ch ) ) {
            send_to_char( "In your dreams, or what?\r\n", ch );
            return;
        }

        for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
            if ( IS_NPC( mob )
                 && ( xIS_SET( mob->act, ACT_PRACTICE ) || xIS_SET( mob->act, ACT_NEWBPRACTICE ) ) )
                break;
        if ( !mob ) {
            send_to_char( "You can't do that here.\r\n", ch );
            return;
        }

        if ( ch->practice <= 0 ) {
            act( AT_TELL, "$n tells you 'You must earn some more practice sessions.'", mob, NULL,
                 ch, TO_VICT );
            return;
        }

        sn = skill_lookup( argument );

        if ( ( sn == -1 ) || ( !IS_NPC( ch ) && !CAN_LEARN( ch, sn, TRUE ) ) ) {
            act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'", mob, NULL, ch,
                 TO_VICT );
            return;
        }

        if ( is_name( skill_tname[skill_table[sn]->type], ( char * ) CANT_PRAC )
             || !can_teach( ch, mob, sn ) ) {
            act( AT_TELL, "\r\n$n tells you 'I don't know how to teach that.'", mob, NULL, ch,
                 TO_VICT );
            send_to_char
                ( "\r\n&WType 'help Teacher' for help finding new spells/skills in the realms.\r\n",
                  ch );
            return;
        }

        /*
         * Skill requires a special teacher - run through the list of teachers
         * Check into the possibility of activating mppractice mobs with practice command.
         */
        if ( VLD_STR( skill_table[sn]->teachers ) ) {
            /*
             * Get the mob vnum here 
             */
            snprintf( buf, MSL, "%d", mob->pIndexData->vnum );
            if ( !is_name( buf, skill_table[sn]->teachers ) ) {
                act( AT_TELL, "\r\n$n tells you, 'I do not know how to teach that.'", mob, NULL, ch,
                     TO_VICT );
                send_to_char
                    ( "\r\n&WType 'help Teacher' for help finding new spells/skills in the realms.\r\n",
                      ch );
                return;
            }
        }

        SKILLTYPE              *skill;

        skill = skill_table[sn];

        highskill = ( get_curr_str( ch ) + get_curr_dex( ch ) ) / 2;
        highspell = ( get_curr_int( ch ) + get_curr_wis( ch ) ) / 2;

        if ( skill->type == SKILL_SKILL )
            usehigh = highskill;
        else if ( skill->type == SKILL_SPELL )
            usehigh = highspell;

        if ( ch->pcdata->learned[sn] > ( get_maxadept( ch, sn, TRUE ) - 6 )
             || ch->pcdata->learned[sn] > 40 ) {
            act_printf( AT_TELL, mob, NULL, ch, TO_VICT,
                        "$n tells you, 'I've taught you everything I can about %s.'",
                        skill_table[sn]->name );
            act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob,
                 NULL, ch, TO_VICT );
        }
        else if ( ( skill->type == SKILL_SKILL || skill->type == SKILL_SPELL )
                  && ( ( usehigh >= 18 && ch->pcdata->learned[sn] >= 45 )
                       || ( ( usehigh > 15 && usehigh < 18 ) && ch->pcdata->learned[sn] >= 40 )
                       || ( ( usehigh < 16 && usehigh > 13 ) && ch->pcdata->learned[sn] >= 35 )
                       || ( usehigh < 14 && ch->pcdata->learned[sn] >= 30 ) ) ) {
            act_printf( AT_TELL, mob, NULL, ch, TO_VICT,
                        "$n tells you, 'I've taught you everything I can about %s.'",
                        skill_table[sn]->name );
            act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob,
                 NULL, ch, TO_VICT );
        }
        else {
            ch->practice--;
            if ( ch->pcdata->learned[sn] <= 0 )
                ch->pcdata->learned[sn] += 10;
            else
                ch->pcdata->learned[sn] += 5;
            act( AT_ACTION, "You practice $T.", ch, NULL, skill_table[sn]->name, TO_CHAR );
            act( AT_ACTION, "$n practices $T.", ch, NULL, skill_table[sn]->name, TO_ROOM );

            // check if skill and str > certain amount max 45 else 35
            if ( skill->type == SKILL_SKILL || skill->type == SKILL_SPELL ) {
                if ( usehigh >= 18 ) {
                    if ( ch->pcdata->learned[sn] >= 40 && ch->pcdata->learned[sn] < 50 ) {
                        ch->pcdata->learned[sn] = 45;
                        act( AT_TELL,
                             "$n tells you. 'You'll have to practice it on your own now...'", mob,
                             NULL, ch, TO_VICT );
                    }
                }
                else if ( usehigh > 15 && usehigh < 18 ) {
                    if ( ch->pcdata->learned[sn] >= 30 && ch->pcdata->learned[sn] < 41 ) {
                        ch->pcdata->learned[sn] = 40;
                        act( AT_TELL,
                             "$n tells you. 'You'll have to practice it on your own now...'", mob,
                             NULL, ch, TO_VICT );
                    }
                }
                else if ( usehigh < 16 && usehigh > 13 ) {
                    if ( ch->pcdata->learned[sn] >= 30 && ch->pcdata->learned[sn] < 41 ) {
                        ch->pcdata->learned[sn] = 35;
                        act( AT_TELL,
                             "$n tells you. 'You'll have to practice it on your own now...'", mob,
                             NULL, ch, TO_VICT );
                    }
                }
                else if ( usehigh < 14 ) {
                    if ( ch->pcdata->learned[sn] >= 20 && ch->pcdata->learned[sn] < 40 ) {
                        ch->pcdata->learned[sn] = 30;
                        act( AT_TELL,
                             "$n tells you. 'You'll have to practice it on your own now...'", mob,
                             NULL, ch, TO_VICT );
                    }
                }
            }
        }
    }
}

void do_wimpy( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    int                     wimpy;

    set_char_color( AT_YELLOW, ch );
    one_argument( argument, arg );
    if ( !str_cmp( arg, "max" ) ) {
        if ( IS_PKILL( ch ) )
            wimpy = ( int ) ( ch->max_hit / 2.25 );
        else
            wimpy = ( int ) ( ch->max_hit / 1.2 );
    }
    else if ( arg[0] == '\0' )
        wimpy = ( int ) ch->max_hit / 5;
    else
        wimpy = atoi( arg );
    if ( wimpy < 0 ) {
        send_to_char( "Your courage exceeds your wisdom.\r\n", ch );
        return;
    }
    if ( IS_PKILL( ch ) && wimpy > ( int ) ch->max_hit / 2.25 ) {
        send_to_char( "Such cowardice ill becomes you.\r\n", ch );
        return;
    }
    else if ( wimpy > ( int ) ch->max_hit / 1.2 ) {
        send_to_char( "Such cowardice ill becomes you.\r\n", ch );
        return;
    }
    ch->wimpy = wimpy;
    ch_printf( ch, "Wimpy set to %d hit points.\r\n", wimpy );
    return;
}

void do_password( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                   *pArg;
    char                   *pwdnew;
    char                   *p;
    char                    cEnd;

    if ( IS_NPC( ch ) )
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' ) {
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' ) {
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: password <new> <again>.\r\n", ch );
        return;
    }

    /*
     * This should stop all the mistyped password problems --Shaddai 
     */
    if ( strcmp( arg1, arg2 ) ) {
        send_to_char( "Passwords don't match try again.\r\n", ch );
        return;
    }
    if ( strlen( arg2 ) < 5 ) {
        send_to_char( "New password must be at least five characters long.\r\n", ch );
        return;
    }
    if ( arg1[0] == '!' || arg2[0] == '!' ) {
        send_to_char( "New password cannot begin with the '!' character.", ch );
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ ) {
        if ( *p == '~' ) {
            send_to_char( "New password not acceptable, try again.\r\n", ch );
            return;
        }
    }

    STRFREE( ch->pcdata->pwd );
    ch->pcdata->pwd = STRALLOC( pwdnew );
    if ( xIS_SET( sysdata.save_flags, SV_PASSCHG ) )
        save_char_obj( ch );
    if ( ch->desc && ch->desc->host[0] != '\0' )
        log_printf( "%s changing password from site %s\n", ch->name, ch->desc->host );
    else
        log_printf( "%s changing thier password with no descriptor!", ch->name );
    send_to_char( "Password changed.\r\n", ch );
    return;
}

void do_socials( CHAR_DATA *ch, char *argument )
{
    if ( argument[0] != '\0' ) {
        char                    arg1[MAX_INPUT_LENGTH];

        argument = one_argument( argument, arg1 );
        if ( !check_social( ch, arg1, argument ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
    }
    else {
        int                     iHash;
        int                     col = 0;
        SOCIALTYPE             *social;

        set_pager_color( AT_PLAIN, ch );
        for ( iHash = 0; iHash < 27; iHash++ )
            for ( social = social_index[iHash]; social; social = social->next ) {
                pager_printf( ch, "%-12s", social->name );
                if ( ++col % 6 == 0 )
                    send_to_pager( "\r\n", ch );
            }
        if ( col % 6 != 0 )
            send_to_pager( "\r\n", ch );
        return;
    }
}

void do_commands( CHAR_DATA *ch, char *argument )
{
    int                     col = 0,
        count = 0;
    bool                    found = FALSE,
        arg = FALSE;
    CMDTYPE                *command;
    int                     hash;

    if ( !ch || IS_NPC( ch ) )
        return;
    set_pager_color( AT_PLAIN, ch );
    if ( VLD_STR( argument ) )
        arg = TRUE;
    for ( hash = 0; hash < 126; hash++ )
        for ( command = command_hash[hash]; command; command = command->next ) {
            if ( command->level <= MAX_LEVEL ) {
                if ( arg && str_prefix( argument, command->name ) )
                    continue;
                if ( ch->level < MAX_LEVEL && command->cshow == FALSE )
                    continue;
                if ( VLD_STR( ch->pcdata->bestowments )
                     && is_name( command->name, ch->pcdata->bestowments ) ) {
                    if ( command->level <= get_trust( ch ) )
                        continue;
                }
                else if ( command->level > get_trust( ch ) )
                    continue;
                if ( arg )
                    found = TRUE;
                count++;
                pager_printf( ch, "%s", command->do_fun == skill_notfound ? "&R[" : " " );
                if ( command->cshow == FALSE )
                    pager_printf( ch, "&R" );
                else if ( command->level > LEVEL_AVATAR )
                    pager_printf( ch, "&C" );
                else
                    pager_printf( ch, "&D" );
                pager_printf( ch, "%-15.15s", command->name );
                pager_printf( ch, "%s", command->do_fun == skill_notfound ? "&R]&D" : " " );
                if ( ++col % 5 == 0 )
                    send_to_pager( "\r\n", ch );
            }
        }
    if ( col % 5 != 0 )
        send_to_pager( "&D\r\n", ch );
    if ( arg && !found )
        ch_printf( ch, "&WNo command found under %s.\r\n", argument );
    else if ( arg )
        ch_printf( ch, "&R%d&D &Wcommand%s found under &C%s&W.&D\r\n", count, count == 1 ? "" : "s",
                   argument );
    else
        ch_printf( ch, "&R%d&D &Wcommand%s.&D\r\n", count, count == 1 ? "" : "s" );
    return;
}

/* display WIZLIST file -Thoric */
void do_wizlist( CHAR_DATA *ch, char *argument )
{
    set_pager_color( AT_IMMORT, ch );
    send_to_char( "The wizlist command is no more.\r\n\r\n", ch );
    send_to_char( "Here at 6 Dragons we use the stafflist command.\r\n", ch );
    return;
}

/* DISPLAY THE WAYFARERS STAFF LIST FILE */
void do_stafflist( CHAR_DATA *ch, char *argument )
{
    set_pager_color( AT_IMMORT, ch );
    show_file( ch, STAFFLIST_FILE );
}

void do_config( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            s1[16],
                            s2[16],
                            s3[16],
                            s4[16];

    if ( !ch || IS_NPC( ch ) )
        return;
    snprintf( s1, 16, "%s", color_str( AT_CONFIG1, ch ) );
    snprintf( s2, 16, "%s", color_str( AT_CONFIG2, ch ) );
    snprintf( s3, 16, "%s", color_str( AT_CONFIG3, ch ) );
    snprintf( s4, 16, "%s", color_str( AT_CONFIG4, ch ) );
    one_argument( argument, arg );
    set_char_color( AT_CONFIG3, ch );
    if ( !VLD_STR( arg ) ) {
        ch_printf( ch,
                   "%s\r\nConfigurations %s[%s"
                   "use 'config <keyword>' to toggle, see 'help config'%s]%s\r\n\r\n", s1, s2, s3,
                   s2, s1 );
        ch_printf( ch, "Display:%s", s2 );
        ch_printf( ch, "\r\n%-14s%s",
                   IS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) ? "PAGER" : "pager", s2 );
        ch_printf( ch, "%-14s%s", IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ? "GAG" : "gag", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_BRIEF ) ? "BRIEF" : "brief", s2 );
        ch_printf( ch, "%-14s%s\r\n", xIS_SET( ch->act, PLR_PROMPT ) ? "PROMPT" : "prompt", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_ANSI ) ? "ANSI" : "ansi", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_RIP ) ? "RIP" : "rip", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_COMBINE ) ? "COMBINE" : "combine", s2 );
        ch_printf( ch, "%-14s%s\r\n", xIS_SET( ch->act, PLR_BLANK ) ? "BLANK" : "blank", s2 );
        ch_printf( ch, "%-14s%s\r\n", xIS_SET( ch->act, PLR_RP ) ? "RP" : "rp", s2 );
        ch_printf( ch, "%-14s%s", IS_SET( ch->pcdata->flags, PCFLAG_SHOWSLOTS ) ? "SLOTS" : "slots",
                   s2 );
        set_char_color( AT_CONFIG1, ch );
        send_to_char( "\r\n\r\nAuto:", ch );
        ch_printf( ch, "\r\n%s%-14s%s", s2,
                   xIS_SET( ch->act, PLR_AUTOTRASH ) ? "AUTOTRASH" : "autotrash", s2 );
        ch_printf( ch, "\r\n%s%-14s%s", s2,
                   xIS_SET( ch->act, PLR_AUTODOOR ) ? "AUTODOOR" : "autodoor", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_AUTOMONEY ) ? "AUTOMONEY" : "automoney",
                   s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_AUTOLOOT ) ? "AUTOLOOT" : "autoloot", s2 );
        ch_printf( ch, "%-14s%s\r\n", xIS_SET( ch->act, PLR_AUTOEXIT ) ? "AUTOEXIT" : "autoexit",
                   s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_AUTOGLANCE ) ? "AUTOGLANCE" : "autoglance",
                   s2 );
        ch_printf( ch, "%-14s%s", IS_SET( ch->pcdata->flags, PCFLAG_HINTS ) ? "HINTS" : "hints",
                   s2 );
        set_char_color( AT_CONFIG1, ch );
        send_to_char( "\r\n\r\nSafeties:", ch );
        ch_printf( ch, "\r\n%s%-12s%s", s2,
                   IS_SET( ch->pcdata->flags, PCFLAG_NORECALL ) ? "NORECALL" : "norecall", s2 );
        ch_printf( ch, "%-14s%s",
                   IS_SET( ch->pcdata->flags, PCFLAG_NOSUMMON ) ? "NOSUMMON" : "nosummon", s2 );
        ch_printf( ch, "%-14s%s", IS_SET( ch->pcdata->flags, PCFLAG_NOBEEP ) ? "NOBEEP" : "nobeep",
                   s2 );
        if ( !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) || IS_IMMORTAL( ch ) ) {
            ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_SHOVEDRAG ) ? "DRAG" : "drag", s2 );
            ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_NICE ) ? "NICE" : "nice", s2 );
        }
        set_char_color( AT_CONFIG1, ch );
        send_to_char( "\r\n\r\nMisc:", ch );
        ch_printf( ch, "\r\n%s%-12s%s", s2,
                   xIS_SET( ch->act, PLR_TELNET_GA ) ? "TELNETGA" : "telnetga", s2 );
        ch_printf( ch, "%-14s%s",
                   IS_SET( ch->pcdata->flags, PCFLAG_GROUPWHO ) ? "GROUPWHO" : "groupwho", s2 );
        ch_printf( ch, "%-14s%s",
                   IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) ? "NOINTRO" : "nointro", s2 );
        ch_printf( ch, "%-14s%s", IS_BLIND( ch ) ? "SCREENREADER" : "screenreader", s2 );
        ch_printf( ch, "\r\n%-14s%s", xIS_SET( ch->act, PLR_MUSIC ) ? "MUSIC" : "music", s2 );
        ch_printf( ch, "%-14s%s", xIS_SET( ch->act, PLR_SOUND ) ? "SOUND" : "sound", s2 );

        set_char_color( AT_CONFIG1, ch );
        send_to_char( "\r\n\r\nSettings:", ch );
        ch_printf( ch, "\r\n%s[%s%d%s] Wimpy", s2, ch->wimpy > 0 ? s4 : s3, ch->wimpy, s2 );
        ch_printf( ch, "\r\n%s[%s%d%s] Pager Length", s2, ch->pcdata->pagerlen > 0 ? s4 : s3,
                   ch->pcdata->pagerlen, s2 );
        ch_printf( ch, "\r\n%sTo use triple experience points type 'config texp'", s2 );

        if ( IS_IMMORTAL( ch ) ) {
            set_char_color( AT_CONFIG1, ch );
            send_to_char( "\r\n\r\n6 Dragons Staff Toggles:", ch );
            ch_printf( ch, "\r\n%s%-12s%s", s2, xIS_SET( ch->act, PLR_STAFF ) ? "STAFF" : "staff",
                       s2 );
        }
        set_char_color( AT_CONFIG1, ch );
        send_to_char( "\r\n\r\nSentences imposed on you(if any):\r\n", ch );
        set_char_color( AT_CONFIG3, ch );
        ch_printf( ch, "\r\n%s%s%s%s%s",
                   xIS_SET( ch->act,
                            PLR_SILENCE ) ?
                   " For your abuse of channels, you are currently silenced.\r\n" : "",
                   xIS_SET( ch->act,
                            PLR_NO_EMOTE ) ? " The gods have removed your emotes.\r\n" :
                   "", xIS_SET( ch->act,
                                PLR_NO_TELL ) ?
                   " You are not permitted to send 'tells' to others.\r\n" : "",
                   xIS_SET( ch->act,
                            PLR_LITTERBUG ) ?
                   " A convicted litterbug.  You cannot drop anything.\r\n" : "",
                   xIS_SET( ch->act,
                            PLR_THIEF ) ?
                   " A proven thief, you will be hunted by the authorities.\r\n" : "",
                   xIS_SET( ch->act,
                            PLR_KILLER ) ?
                   " For the crime of murder you are sentenced to death...\r\n" : "" );
    }
    else {
        int                     bit = 0;

        if ( !str_prefix( arg, "texp" ) ) {
            if ( ch->pcdata->getsdoubleexp == TRUE ) {
                send_to_char( "You are already into your hour of triple experience!\r\n", ch );
                return;
            }

            if ( ch->pcdata->double_exp_timer >= 100 ) {
                ch->pcdata->double_exp_timer -= 100;
                ch->pcdata->getsdoubleexp = TRUE;
                ch->pcdata->last_dexpupdate = current_time;
                send_to_char( "Your hour of triple experience has begun.\r\n", ch );
            }
            else
                send_to_char
                    ( "You don't have enough points to get an hour of triple experience.\r\n", ch );
            return;
        }
        if ( !str_prefix( arg, "autoexit" ) )
            bit = PLR_AUTOEXIT;
        else if ( !str_prefix( arg, "autoloot" ) )
            bit = PLR_AUTOLOOT;
/*    else if(!str_prefix(arg, "automap"))
      bit = PLR_AUTOMAP; */
/*
    else if(!str_prefix(arg, "showresources"))
      bit = PLR_RESOURCE;
*/
        else if ( !str_prefix( arg, "autotrash" ) )
            bit = PLR_AUTOTRASH;
        else if ( !str_prefix( arg, "autodoor" ) )
            bit = PLR_AUTODOOR;
        else if ( !str_prefix( arg, "automoney" ) )
            bit = PLR_AUTOMONEY;
        else if ( !str_prefix( arg, "autoglance" ) )
            bit = PLR_AUTOGLANCE;
        else if ( !str_prefix( arg, "blank" ) )
            bit = PLR_BLANK;
        else if ( !str_prefix( arg, "rp" ) )
            bit = PLR_RP;
        else if ( !str_prefix( arg, "brief" ) )
            bit = PLR_BRIEF;
        else if ( !str_prefix( arg, "combine" ) )
            bit = PLR_COMBINE;
        else if ( !str_prefix( arg, "prompt" ) )
            bit = PLR_PROMPT;
        else if ( !str_prefix( arg, "telnetga" ) )
            bit = PLR_TELNET_GA;
        else if ( !str_prefix( arg, "ansi" ) )
            bit = PLR_ANSI;
        else if ( !str_prefix( arg, "rip" ) )
            bit = PLR_RIP;
        else if ( !str_prefix( arg, "nice" ) )
            bit = PLR_NICE;
        else if ( !str_prefix( arg, "drag" ) )
            bit = PLR_SHOVEDRAG;
        else if ( !str_prefix( arg, "screenreader" ) )
            bit = PLR_BLIND;
        else if ( !str_prefix( arg, "music" ) )
            bit = PLR_MUSIC;
        else if ( !str_prefix( arg, "sound" ) )
            bit = PLR_SOUND;
        else if ( IS_IMMORTAL( ch ) && !str_prefix( arg, "staff" ) )
            bit = PLR_STAFF;

        if ( bit ) {
            if ( ( bit == PLR_FLEE || bit == PLR_NICE || bit == PLR_SHOVEDRAG )
                 && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
                send_to_char( "Pkill characters can not config that option.\r\n", ch );
                return;
            }
            xTOGGLE_BIT( ch->act, bit );
            ch_printf( ch, "%s has been turned %s.\r\n", capitalize( arg ),
                       xIS_SET( ch->act, bit ) ? "&Gon&D" : "&Roff&D" );
            return;
        }
        else {
            if ( !str_prefix( arg, "norecall" ) )
                bit = PCFLAG_NORECALL;
            else if ( !str_prefix( arg, "nointro" ) )
                bit = PCFLAG_NOINTRO;
            else if ( !str_prefix( arg, "nosummon" ) )
                bit = PCFLAG_NOSUMMON;
            else if ( !str_prefix( arg, "gag" ) )
                bit = PCFLAG_GAG;
            else if ( !str_prefix( arg, "hints" ) )
                bit = PCFLAG_HINTS;
            else if ( !str_prefix( arg, "slots" ) )
                bit = PCFLAG_SHOWSLOTS;
            else if ( !str_prefix( arg, "pager" ) )
                bit = PCFLAG_PAGERON;
            else if ( !str_prefix( arg, "groupwho" ) )
                bit = PCFLAG_GROUPWHO;
            else if ( !str_prefix( arg, "@hgflag_" ) )
                bit = PCFLAG_HIGHGAG;
            else if ( !str_prefix( arg, "nobeep" ) )
                bit = PCFLAG_NOBEEP;
            else {
                send_to_char( "Config which option?\r\n", ch );
                return;
            }
            TOGGLE_BIT( ch->pcdata->flags, bit );
            ch_printf( ch, "%s has been turned %s.\r\n", capitalize( arg ),
                       IS_SET( ch->pcdata->flags, bit ) ? "&Gon&D" : "&Roff&D" );
            return;
        }
    }
    return;
}

void do_credits( CHAR_DATA *ch, char *argument )
{
    do_help( ch, ( char * ) "credits" );
}

extern int              top_area;

/* New do_areas, written by Fireblade, last modified - 4/27/97
 *   Syntax: area            ->      lists areas in alphanumeric order
 *           area <a>        ->      lists areas with soft max less than parameter a
 *           area <a> <b>    ->      lists areas with soft max bewteen numbers a and b
 *           area old        ->      list areas in order loaded
 */

void do_areas( CHAR_DATA *ch, char *argument )
{
    AREA_DATA              *pArea;
    const char             *header_string1 =
        "&c|&C    Authors&c    |&C              Areas             &c |&C Soft Ranges&c |&C Hard Ranges &c|\r\n";
    const char             *header_string2 =
        "&c+---------------+---------------------------------+-------------+-------------+\r\n";
    const char             *print_string =
        "&c|&C %-13s&c |&C %-31s&c |&C   %-3.2d-%3.2d&c   |&C   %-3.2d-%3.2d&c   |\r\n";
    const char             *print_string2 =
        "&c|&C %-13s&c |&C %-31s&c |&C   %-3.2d-%3.2d&c   |&C   %-3.2d-%3.2d&c   |&R Group&c\r\n";
    const char             *info_string =
        "\r\n  &cArea Name   : &C%s\r\n&b--=--=--=--=--=--=--=--=--=--=--\r\n  &cAuthor      : &C%s\r\n  &cDerivatives : &C%s\r\n  &cHomeland    : &C%s\r\n  &cHard Range  : &C%3d-%-3d\r\n  &cSoft Range  :&C %3d-%-3d\r\n&cArea Description\r\n&b--=--=--=--=--=--=--=--=--=--=--&C\r\n%s";
    char                    arg[MSL];
    int                     lower_bound = 0;
    int                     upper_bound = MAX_LEVEL + 1;

    argument = one_argument( argument, arg );

    if ( arg && arg[0] != '\0' ) {
        if ( !is_number( arg ) ) {
            if ( !strcmp( arg, "info" ) ) {
                AREA_DATA              *temp = NULL;

                if ( !argument || argument[0] == '\0' ) {
                    send_to_char( "\r\nSyntax: areas info <area name>\r\n", ch );
                    return;
                }

                for ( temp = first_area; temp != NULL; temp = temp->next ) {
                    if ( !str_cmp( argument, temp->name ) )
                        break;
                }

                if ( temp != NULL ) {
                    pager_printf_color( ch, info_string, temp->name, temp->author,
                                        ( !temp->derivatives
                                          || temp->derivatives[0] ==
                                          '\0' ) ? "None" : temp->derivatives,
                                        ( !temp->htown
                                          || temp->htown[0] ==
                                          '\0' ) ? "None" : temp->htown,
                                        temp->low_hard_range, temp->hi_hard_range,
                                        temp->low_soft_range, temp->hi_soft_range, ( !temp->desc
                                                                                     || temp->
                                                                                     desc[0] ==
                                                                                     '\0' ) ? "None"
                                        : temp->desc );
                }
                else {
                    send_to_char( "\r\nSyntax: areas info <area name>\r\n", ch );
                    return;
                }
                return;
            }
            else if ( !str_cmp( arg, "old" ) ) {
                set_pager_color( AT_PLAIN, ch );

                send_to_pager( "\r\n", ch );
                send_to_pager( header_string2, ch );
                send_to_pager( header_string1, ch );
                send_to_pager( header_string2, ch );

                for ( pArea = first_area; pArea; pArea = pArea->next )
                    if ( !IS_SET( pArea->flags, AFLAG_UNOTSEE ) ) {
                        pager_printf( ch, print_string, pArea->author, pArea->name,
                                      pArea->low_soft_range, pArea->hi_soft_range,
                                      pArea->low_hard_range, pArea->hi_hard_range );
                    }

                send_to_pager( header_string2, ch );
                send_to_pager
                    ( "For detailed information about a specific area, type : &Careas info <area name>\r\n",
                      ch );

                return;
            }
            else if ( !str_cmp( arg, "group" ) ) {
                set_pager_color( AT_PLAIN, ch );

                send_to_pager( "\r\n", ch );
                send_to_pager( header_string2, ch );
                send_to_pager( header_string1, ch );
                send_to_pager( header_string2, ch );

                for ( pArea = first_area_name; pArea; pArea = pArea->next_sort_name ) {
                    if ( IS_SET( pArea->flags, AFLAG_UNOTSEE ) )
                        continue;
                    if ( !IS_SET( pArea->flags, AFLAG_GROUP ) )
                        continue;
                    pager_printf( ch, print_string2, pArea->author, pArea->name,
                                  pArea->low_soft_range, pArea->hi_soft_range,
                                  pArea->low_hard_range, pArea->hi_hard_range );
                }

                send_to_pager( header_string2, ch );
                return;
            }
            else {
                send_to_char( "Syntax: areas\r\n", ch );
                send_to_char( "Syntax: areas old\r\n", ch );
                send_to_char( "Syntax: areas group\r\n", ch );
                send_to_char( "Syntax: areas <#> [<#>]\r\n", ch );
                send_to_char( "Syntax: areas info <area name>\r\n", ch );
                return;
            }
        }

        upper_bound = atoi( arg );
        lower_bound = upper_bound;

        argument = one_argument( argument, arg );

        if ( arg && arg[0] != '\0' ) {
            if ( !is_number( arg ) ) {
                send_to_char( "Area may only be followed by numbers.\r\n", ch );
                return;
            }

            upper_bound = atoi( arg );

            argument = one_argument( argument, arg );

            if ( arg && arg[0] != '\0' ) {
                send_to_char( "Only two level numbers allowed.\r\n", ch );
                return;
            }
        }
    }

    if ( lower_bound > upper_bound ) {
        int                     swap = lower_bound;

        lower_bound = upper_bound;
        upper_bound = swap;
    }

    set_pager_color( AT_PLAIN, ch );

    send_to_pager( "\r\n", ch );
    send_to_pager( header_string2, ch );
    send_to_pager( header_string1, ch );
    send_to_pager( header_string2, ch );

    for ( pArea = first_area_name; pArea; pArea = pArea->next_sort_name ) {
        if ( !IS_SET( pArea->flags, AFLAG_UNOTSEE ) )
            if ( pArea->hi_soft_range >= lower_bound && pArea->low_soft_range <= upper_bound ) {
                if ( IS_SET( pArea->flags, AFLAG_GROUP ) ) {
                    pager_printf( ch, print_string2, pArea->author, pArea->name,
                                  pArea->low_soft_range, pArea->hi_soft_range,
                                  pArea->low_hard_range, pArea->hi_hard_range );
                }
                else
                    pager_printf( ch, print_string, pArea->author, pArea->name,
                                  pArea->low_soft_range, pArea->hi_soft_range,
                                  pArea->low_hard_range, pArea->hi_hard_range );
            }
    }

    send_to_pager( header_string2, ch );
    send_to_pager
        ( "&RGroup&c next to the area level ranges means the area is intended for a group of players.\r\n",
          ch );
    send_to_pager
        ( "&cFor those nostalgic players, stock areas are available through the &Wtime machine&c\r\nquest in Gnome Tower area.\r\n",
          ch );
}

void do_afk( CHAR_DATA *ch, char *argument )
{
    char                    buf[MIL];

    if ( !ch || IS_NPC( ch ) )
        return;
    set_char_color( AT_WHITE, ch );
    if ( xIS_SET( ch->act, PLR_AFK ) ) {
        xREMOVE_BIT( ch->act, PLR_AFK );
        send_to_char( "You are no longer afk.\r\n", ch );
        if ( ch->pcdata->afkbuf )
            STRFREE( ch->pcdata->afkbuf );
        act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_ROOM );
    }
    else {
        xSET_BIT( ch->act, PLR_AFK );
        if ( ch->pcdata->afkbuf )
            STRFREE( ch->pcdata->afkbuf );
        if ( VLD_STR( argument ) ) {
            if ( isalpha( argument[0] ) || isdigit( argument[0] ) ) {
                buf[0] = ' ';
                mudstrlcpy( buf + 1, argument, MIL - 1 );
            }
            else
                mudstrlcpy( buf, argument, MIL );
            ch->pcdata->afkbuf = STRALLOC( buf );
        }
        if ( VLD_STR( ch->pcdata->afkbuf ) ) {
            act_printf( AT_GREY, ch, NULL, NULL, TO_ROOM, "$n is now afk -%s", ch->pcdata->afkbuf );
            ch_printf( ch, "You are now afk -%s.\r\n", ch->pcdata->afkbuf );
        }
        else {
            act( AT_GREY, "$n is now afk.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You are now afk.\r\n", ch );
        }
    }
    return;
}

void do_slist( CHAR_DATA *ch, char *argument )
{
    int                     sn,
                            i,
                            lFound;
    char                    skn[MAX_INPUT_LENGTH];
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    int                     lowlev = 1,
        hilev = MAX_LEVEL,
        cl = 0,
        findclass = -2;
    short                   lasttype = SKILL_SPELL;

    bool                    Class = FALSE;

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

/* Need to allow players to either write - 'slist 1 10', 'slist necromancer', 'slist druid 1 20' */

    if ( argument[0] != '\0' ) {
        Class = TRUE;
        lowlev = atoi( arg2 );
        hilev = atoi( argument );
    }
    else {                                             /* no third argument - ie slist 1
                                                        * * * * * 10 or slist druid 1 */

        if ( arg2[0] != '\0' ) {                       /* See comment above */
            if ( is_number( arg1 ) ) {
                lowlev = atoi( arg1 );
                hilev = atoi( arg2 );
            }
            else {
                Class = TRUE;
                lowlev = atoi( arg2 );
            }
        }
        else {                                         /* no 2nd argument - ie slist * *
                                                        * * * necromancer, slist 3 */

            if ( arg1[0] != '\0' ) {
                if ( is_number( arg1 ) )
                    lowlev = atoi( arg1 );
                else
                    Class = TRUE;
            }
        }
    }

    if ( ( lowlev < 1 ) || ( lowlev > LEVEL_IMMORTAL ) )
        lowlev = 1;

    if ( ( hilev < 0 ) || ( hilev >= LEVEL_IMMORTAL ) )
        hilev = LEVEL_HERO;

    if ( lowlev > hilev )
        lowlev = hilev;

    if ( Class ) {
        for ( cl = 0; cl < MAX_CLASS && class_table[cl]; cl++ )
            if ( !str_cmp( class_table[cl]->who_name, arg1 ) )
                break;
    }
    if ( Class && cl >= MAX_CLASS ) {
        send_to_char( "No such class.\r\n", ch );
        return;
    }

/* That should cover everything! Now we have a CLASS variable that is TRUE if we're checking another class out, and a hilev/lowlev. */

    set_pager_color( AT_MAGIC, ch );
    send_to_pager( "SPELL & SKILL LIST\r\n", ch );
    if ( !IS_BLIND( ch ) ) {
        send_to_pager( "------------------\r\n", ch );
    }
    for ( i = lowlev; i <= hilev; i++ ) {
        lFound = 0;
        snprintf( skn, MAX_INPUT_LENGTH, "%s", "Spell" );
        for ( sn = 0; sn < top_sn; sn++ ) {
            if ( !skill_table[sn]->name )
                break;

            if ( skill_table[sn]->type != lasttype ) {
                lasttype = skill_table[sn]->type;
                mudstrlcpy( skn, skill_tname[lasttype], MAX_INPUT_LENGTH );
            }

            if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
                continue;

            if ( !Class && ( i == skill_table[sn]->skill_level[ch->Class]
                             || ( IS_SECONDCLASS( ch )
                                  && ( i == skill_table[sn]->skill_level[ch->secondclass]
                                       || ( IS_THIRDCLASS( ch )
                                            && i ==
                                            skill_table[sn]->skill_level[ch->thirdclass] ) ) ) ) ) {
                if ( !lFound ) {
                    lFound = 1;
                    pager_printf( ch, "Level %d\r\n", i );
                }
                if ( !IS_SECONDCLASS( ch ) )
                    pager_printf( ch, "%7s: %30.30s  Current/Max: %3d/%-3d\r\n", skn,
                                  skill_table[sn]->name, ch->pcdata->learned[sn], get_maxadept( ch,
                                                                                                sn,
                                                                                                FALSE ) );
                else {
                    findclass =
                        ( i == skill_table[sn]->skill_level[ch->Class] ) ? ch->Class : ( i ==
                                                                                         skill_table
                                                                                         [sn]->
                                                                                         skill_level
                                                                                         [ch->
                                                                                          secondclass] )
                        ? ch->secondclass : ch->thirdclass;
                    pager_printf( ch, "&D%4.2s (%-5.5s): %15.15s \t &DCurrent/Max: %3d/%-3d&D\r\n",
                                  skn,
                                  findclass < -1 ? "Unknown" : findclass ==
                                  -1 ? "Racial" : class_table[findclass]->who_name,
                                  skill_table[sn]->name, ch->pcdata->learned[sn], get_maxadept( ch,
                                                                                                sn,
                                                                                                FALSE ) );
                }
            }

            if ( Class && i == skill_table[sn]->skill_level[cl] ) { /* CLASS exists so
                                                                     * we're * * using *
                                                                     * an argument, * ie
                                                                     * * slist
                                                                     * necromancer */
                if ( !lFound ) {
                    lFound = 1;
                    pager_printf( ch, "Level %d\r\n", i );
                }

                pager_printf( ch, "&D%7s: %20.20s \t                  Current/Max: %3d/%-3d&D\r\n",
                              skn, skill_table[sn]->name, ch->pcdata->learned[sn],
                              skill_table[sn]->skill_adept[cl] );
            }

        }
    }
    return;
}

void do_pager( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;
    set_char_color( AT_NOTE, ch );
    argument = one_argument( argument, arg );
    if ( !*arg ) {
        if ( IS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) ) {
            send_to_char( "Pager disabled.\r\n", ch );
            do_config( ch, ( char * ) "-pager" );
        }
        else {
            ch_printf( ch, "Pager is now enabled at %d lines.\r\n", ch->pcdata->pagerlen );
            do_config( ch, ( char * ) "+pager" );
        }
        return;
    }
    if ( !is_number( arg ) ) {
        send_to_char( "Set page pausing to how many lines?\r\n", ch );
        return;
    }
    ch->pcdata->pagerlen = atoi( arg );
    if ( ch->pcdata->pagerlen < 5 )
        ch->pcdata->pagerlen = 5;
    ch_printf( ch, "Page pausing set to %d lines.\r\n", ch->pcdata->pagerlen );
    return;
}

/*
 * The ignore command allows players to ignore up to MAX_IGN
 * other players. Players may ignore other characters whether
 * they are online or not. This is to prevent people from
 * spamming someone and then logging off quickly to evade
 * being ignored.
 * Syntax:
 *  ignore  -  lists players currently ignored
 *  ignore none  -  sets it so no players are ignored
 *  ignore <player>  -  start ignoring player if not already
 *     ignored otherwise stop ignoring player
 *  ignore reply  -  start ignoring last player to send a
 *     tell to ch, to deal with invis spammers
 * Last Modified: June 26, 1997
 * - Fireblade
 */
void do_ignore( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    IGNORE_DATA            *temp,
                           *next;
    char                    fname[1024];
    struct stat             fst;
    CHAR_DATA              *victim;

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );

    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( arg[0] ), capitalize( arg ) );

    victim = NULL;

    /*
     * If no arguements, then list players currently ignored 
     */
    if ( arg[0] == '\0' ) {
        set_char_color( AT_DIVIDER, ch );
        ch_printf( ch, "\r\n----------------------------------------\r\n" );
        set_char_color( AT_DGREEN, ch );
        ch_printf( ch, "You are currently ignoring:\r\n" );
        set_char_color( AT_DIVIDER, ch );
        ch_printf( ch, "----------------------------------------\r\n" );
        set_char_color( AT_IGNORE, ch );

        if ( !ch->pcdata->first_ignored ) {
            ch_printf( ch, "\t    no one\r\n" );
            return;
        }

        for ( temp = ch->pcdata->first_ignored; temp; temp = temp->next ) {
            ch_printf( ch, "\t  - %s\r\n", temp->name );
        }

        return;
    }
    /*
     * Clear players ignored if given arg "none" 
     */
    else if ( !strcmp( arg, "none" ) ) {
        for ( temp = ch->pcdata->first_ignored; temp; temp = next ) {
            next = temp->next;
            UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
            STRFREE( temp->name );
            DISPOSE( temp );
        }

        set_char_color( AT_IGNORE, ch );
        ch_printf( ch, "You now ignore no one.\r\n" );

        return;
    }
    /*
     * Prevent someone from ignoring themself... 
     */
    else if ( !strcmp( arg, "self" ) || nifty_is_name( arg, ch->name ) ) {
        set_char_color( AT_IGNORE, ch );
        ch_printf( ch, "Did you type something?\r\n" );
        return;
    }
    else {
        int                     i;

        /*
         * get the name of the char who last sent tell to ch 
         */
        if ( !strcmp( arg, "reply" ) ) {
            if ( !ch->reply ) {
                set_char_color( AT_IGNORE, ch );
                ch_printf( ch, "They're not here.\r\n" );
                return;
            }
            else {
                mudstrlcpy( arg, ch->reply->name, MIL );
            }
        }

        /*
         * Loop through the linked list of ignored players 
         */
        /*
         * keep track of how many are being ignored     
         */
        for ( temp = ch->pcdata->first_ignored, i = 0; temp; temp = temp->next, i++ ) {
            /*
             * If the argument matches a name in list remove it 
             */
            if ( !strcmp( temp->name, capitalize( arg ) ) ) {
                UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
                set_char_color( AT_IGNORE, ch );
                ch_printf( ch, "You no longer ignore %s.\r\n", temp->name );
                STRFREE( temp->name );
                DISPOSE( temp );
                return;
            }
        }

        /*
         * if there wasn't a match check to see if the name   
         */
        /*
         * is valid. This if-statement may seem like overkill 
         */
        /*
         * but it is intended to prevent people from doing the
         */
        /*
         * spam and log thing while still allowing ya to      
         */
        /*
         * ignore new chars without pfiles yet...             
         */
        if ( stat( fname, &fst ) == -1
             && ( !( victim = get_char_world( ch, arg ) ) || IS_NPC( victim )
                  || strcmp( capitalize( arg ), victim->name ) != 0 ) ) {
            set_char_color( AT_IGNORE, ch );
            ch_printf( ch, "No player exists by that" " name.\r\n" );
            return;
        }

        if ( victim ) {
            mudstrlcpy( capitalize( arg ), victim->name, MIL );
        }

        if ( !check_parse_name( capitalize( arg ), TRUE ) ) {
            set_char_color( AT_IGNORE, ch );
            send_to_char( "No player exists by that name.\r\n", ch );
            return;
        }

        /*
         * If its valid and the list size limit has not been 
         */
        /*
         * reached create a node and at it to the list       
         */
        if ( i < MAX_IGN ) {
            IGNORE_DATA            *lnew;

            CREATE( lnew, IGNORE_DATA, 1 );
            lnew->name = STRALLOC( capitalize( arg ) );
            lnew->next = NULL;
            lnew->prev = NULL;
            LINK( lnew, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
            set_char_color( AT_IGNORE, ch );
            ch_printf( ch, "You now ignore %s.\r\n", lnew->name );
            return;
        }
        else {
            set_char_color( AT_IGNORE, ch );
            ch_printf( ch, "You may only ignore %d players.\r\n", MAX_IGN );
            return;
        }
    }
}

/*
 * This function simply checks to see if ch is ignoring ign_ch.
 * Last Modified: October 10, 1997
 * - Fireblade
 */
bool is_ignoring( CHAR_DATA *ch, CHAR_DATA *ign_ch )
{
    IGNORE_DATA            *temp;

    if ( IS_NPC( ch ) || IS_NPC( ign_ch ) )
        return FALSE;

    for ( temp = ch->pcdata->first_ignored; temp; temp = temp->next ) {
        if ( nifty_is_name( temp->name, ign_ch->name ) )
            return TRUE;
    }

    return FALSE;
}

/*
 * Codebase Version Information - Orion Elder
 */
void do_version( CHAR_DATA *ch, char *argument )
{
    set_pager_color( CODEBASE_VERSION_CBAR, ch );
    pager_printf_color( ch, "\r\n%s\r\n",
                        strlen( CODEBASE_VERSION_BAR ) > 0 ? CODEBASE_VERSION_BAR : "--==<0>==--" );
    set_pager_color( CODEBASE_VERSION_COLOR, ch );

    pager_printf_color( ch, "\r\n  %s Codebase\r\n    Version: %s.%s\r\n", CODEBASE_VERSION_TITLE,
                        CODEBASE_VERSION_MAJOR, CODEBASE_VERSION_MINOR );

    if ( IS_IMMORTAL( ch ) ) {
        pager_printf_color( ch, "    Compiled on %s at %s\r\n", __DATE__, __TIME__ );
    }

    set_pager_color( CODEBASE_VERSION_CBAR, ch );
    pager_printf_color( ch, "\r\n%s\r\n",
                        strlen( CODEBASE_VERSION_BAR ) > 0 ? CODEBASE_VERSION_BAR : "--==<0>==--" );
    set_pager_color( CODEBASE_VERSION_COLOR, ch );

    pager_printf_color( ch, "\r\n\t%s\r\n",
                        strlen( CODEBASE_VERSION_INFO ) >
                        0 ? CODEBASE_VERSION_INFO : "\r\n\tN//A\r\n" );

    set_pager_color( CODEBASE_VERSION_CBAR, ch );
    pager_printf_color( ch, "\r\n%s\r\n",
                        strlen( CODEBASE_VERSION_BAR ) > 0 ? CODEBASE_VERSION_BAR : "--==<0>==--" );
    set_pager_color( CODEBASE_VERSION_COLOR, ch );

    return;
}

/* Removes anything thats causing flying/floating and sends a message if it wasnt innate.*/
void do_land( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA            *paf,
                           *paf_next;
    char                   *msg = ( char * ) "in a swirl of dust";
    bool                    found = FALSE;

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;
        if ( xIS_SET( paf->bitvector, AFF_FLYING ) || xIS_SET( paf->bitvector, AFF_FLOATING ) ) {
            affect_remove( ch, paf );
            found = TRUE;
        }
    }

    if ( ch->in_room && ( ch->in_room->sector_type == SECT_WATER_SWIM
                          || ch->in_room->sector_type == SECT_WATER_NOSWIM
                          || ch->in_room->sector_type == SECT_RIVER
                          || ch->in_room->sector_type == SECT_WATERFALL
                          || ch->in_room->sector_type == SECT_LAKE ) )
        msg = ( char * ) "in the water making a small splash";

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        if ( ch->race == RACE_PIXIE ) {
            act( AT_GREY, "You fold your wings and land gently $T.", ch, NULL, msg, TO_CHAR );
            act( AT_GREY, "$n folds $s wings and lands gently $T.", ch, NULL, msg, TO_ROOM );
        }
        else {
            act( AT_GREY, "You land gently $T.", ch, NULL, msg, TO_CHAR );
            act( AT_GREY, "$n lands gently $T.", ch, NULL, msg, TO_ROOM );
        }
        xREMOVE_BIT( ch->affected_by, AFF_FLYING );
        xREMOVE_BIT( ch->affected_by, AFF_FLOATING );
        return;
    }

    if ( found ) {
        if ( ch->race == RACE_DRAGON ) {
            if ( can_use_skill( ch, number_percent(  ), gsn_wings ) ) {
                act( AT_GREY, "You fold your wings and land gently $T.", ch, NULL, msg, TO_CHAR );
                act( AT_GREY, "$n folds $s wings and lands gently $T.", ch, NULL, msg, TO_ROOM );
            }
            else {
                act( AT_GREY, "You fold your wings, but come down too fast!", ch, NULL, NULL,
                     TO_CHAR );
                if ( !str_cmp( msg, "in a swirl of dust" ) ) {
                    send_to_char( "&RYou hit the ground hard!\r\n", ch );
                    act( AT_GREY, "$n folds $s wings, but comes down too fast striking the ground!",
                         ch, NULL, NULL, TO_ROOM );
                }
                else {
                    send_to_char( "&RYou make a huge splash!\r\n", ch );
                    act( AT_GREY,
                         "$n folds $s wings, but comes down too fast making a huge splash!", ch,
                         NULL, NULL, TO_ROOM );
                }
                global_retcode = damage( ch, ch, number_range( 10, 20 ), TYPE_HIT );
            }
            return;
        }

        act( AT_GREY, "You land gently $T.", ch, NULL, msg, TO_CHAR );
        act( AT_GREY, "$n lands gently $T.", ch, NULL, msg, TO_ROOM );

        if ( !IS_NPC( ch ) ) {
            if ( ch->race == RACE_VAMPIRE ) {
                act( AT_CYAN, "Your wings shrivel up to small appendages and fall off.", ch, NULL,
                     NULL, TO_CHAR );
                act( AT_CYAN, "$n's wings shrivel up to small appendages and fall off.", ch, NULL,
                     NULL, TO_ROOM );
                ch->pcdata->hp_balance = 0;
            }
        }
        return;
    }

    send_to_char( "&wYou aren't flying.&D\r\n", ch );
    return;
}

void do_eye( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    bool                    found;
    char                    arg[MIL],
                            buf[MSL];
    ROOM_INDEX_DATA        *location,
                           *original;
    EXIT_DATA              *xit;
    int                     edir = 0;

    argument = one_argument( argument, arg );
    if ( !IS_AFFECTED( ch, AFF_WIZARD_EYE ) ) {
        send_to_char( "You are not controlling a wizard eye.\r\n", ch );
        return;
    }
    if ( str_cmp( arg, "north" ) && str_cmp( arg, "n" ) && str_cmp( arg, "south" )
         && str_cmp( arg, "s" ) && str_cmp( arg, "up" ) && str_cmp( arg, "u" )
         && str_cmp( arg, "down" ) && str_cmp( arg, "d" ) && str_cmp( arg, "northwest" )
         && str_cmp( arg, "nw" ) && str_cmp( arg, "southwest" ) && str_cmp( arg, "sw" )
         && str_cmp( arg, "northeast" ) && str_cmp( arg, "ne" )
         && str_cmp( arg, "southeast" ) && str_cmp( arg, "se" )
         && str_cmp( arg, "east" ) && str_cmp( arg, "e" ) && str_cmp( arg, "west" )
         && str_cmp( arg, "w" ) && str_cmp( arg, "look" ) && str_cmp( arg, "l" )
         && str_cmp( arg, "release" ) && str_cmp( arg, "r" ) ) {
        send_to_char( "Syntax: eye <command>\r\n", ch );
        send_to_char( "Commands [ north, east, west, south, northeast, northwest ]\r\n", ch );
        send_to_char( "         [ southeast, southwest, up, down, look, release  ]\r\n", ch );
        return;
    }
    snprintf( buf, MSL, "%s's wizard eye", ch->name );
    found = FALSE;
    for ( victim = first_char; victim; victim = victim->next ) {
        if ( !VLD_STR( victim->short_descr ) )
            continue;
        if ( ( !str_cmp( capitalize( buf ), victim->short_descr ) ) ) {
            found = TRUE;
            break;
        }
    }
    if ( !found ) {
        send_to_char( "You failed.\r\n", ch );
        return;
    }
    if ( ch->move < 1 && str_cmp( arg, "look" ) && str_cmp( arg, "l" ) ) {
        send_to_char( "You need to rest some before you can move your wizard eye.\r\n", ch );
        return;
    }
    else if ( ch->mana < 2 && ( !str_cmp( arg, "look" ) || !str_cmp( arg, "l" ) ) ) {
        send_to_char( "You need to recover some before you can look with your wizard eye.\r\n",
                      ch );
        return;
    }
    else if ( !str_cmp( arg, "look" ) || !str_cmp( arg, "l" ) ) {
        location = victim->in_room;
        original = ch->in_room;
        char_from_room( ch );
        char_to_room( ch, location );
        do_look( ch, ( char * ) "auto" );
        char_from_room( ch );
        char_to_room( ch, original );
        ch->mana -= 2;
        return;                                        /* Return here so it doesnt take
                                                        * moves */
    }
    else if ( !str_cmp( arg, "release" ) || !str_cmp( arg, "r" ) ) {
        send_to_char( "You shutdown your flow of mana and concentration to the wizard eye.\r\n",
                      ch );
        act( AT_MAGIC, "$n blinks out of existance as its power source wanes.", victim, NULL, NULL,
             TO_ROOM );
        extract_char( victim, TRUE );
        return;                                        /* Return here so it doesnt take
                                                        * moves */
    }
    /*
     * Now check for the direction and all 
     */
    location = victim->in_room;
    edir = get_dir( arg );
    xit = get_exit( location, edir );
    /*
     * Valid direction? 
     */
    if ( !xit ) {
        send_to_char( "No exit in that direction.\r\n", ch );
        return;
    }
    move_char( victim, xit, 0 );
    ch_printf( ch, "You concentrate on your wizard eye moving %s.\r\n", dir_name[edir] );
    ch->move -= 1;
    return;
}

void do_clear( CHAR_DATA *ch, char *argument )
{
    send_to_char( "But there are no commands to clear!\r\n", ch );
    return;
}

void do_vomit( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *vomit = create_object( get_obj_index( OBJ_VNUM_VOMIT ), 0 );
    int                     dam;

    if ( IS_NPC( ch ) ) {
        return;
    }

    if ( IS_BLOODCLASS( ch ) ) {
        if ( ch->blood < ( ch->max_blood / 5 ) ) {
            send_to_char( "You can't vomit when you are starved for blood!\r\n", ch );
            return;
        }
    }
    else {
        if ( ch->pcdata->condition[COND_FULL] < 10 ) {
            send_to_char( "You are too hungry to vomit!\r\n", ch );
            return;
        }
        if ( ch->pcdata->condition[COND_THIRST] < 10 ) {
            send_to_char( "You are too thirsty to vomit!\r\n", ch );
            return;
        }
    }

    send_to_char( "You stick a finger down your throat and convulse as you vomit everywhere!\r\n",
                  ch );
    obj_to_room( vomit, ch->in_room );
    vomit->timer = 5;
    ch->pcdata->condition[COND_FULL] -= 20;
    ch->pcdata->condition[COND_THIRST] -= 20;

    if ( !IS_IMMORTAL( ch ) ) {
        dam = ( ( ch->hit ) / 5 );
        if ( ch->hit > 1 )
            damage( ch, ch, dam, TYPE_UNDEFINED );
        if ( ch->mental_state < 50 )
            ch->mental_state += 5;

        if ( IS_BLOODCLASS( ch ) && number_percent(  ) > 75 ) {
            send_to_char( "You lose vital blood, and feel drained of strength.\r\n", ch );
            dam = ( ( ch->max_blood ) / 10 );
            ch->blood -= dam;
            vomit = create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 );
            obj_to_room( vomit, ch->in_room );
            if ( ch->blood < 1 )
                ch->blood = 1;
            ch->pcdata->condition[COND_FULL] = 20;
            ch->pcdata->condition[COND_THIRST] = 20;
        }
    }

    return;
}

void do_tutorial( CHAR_DATA *ch, char *argument )
{
    if ( ch->level > 5 ) {
        send_to_char( "You are too high level to enter the mud school.\r\n", ch );
        return;
    }
    act( AT_YELLOW, "\r\nYou disappear in a spray of shooting light.\r\n\r\n", ch, NULL, ch,
         TO_CHAR );
    act( AT_YELLOW, "$n disappears in a spray of shooting light.", ch, NULL, ch, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, get_room_index( 153 ) );
    set_position( ch, POS_SLEEPING );
    xSET_BIT( ch->act, PLR_TUTORIAL );
    send_to_char( "\r\n\r\n&REverything fades to black, and you lose consiousness.\r\n\r\n", ch );
    act( AT_ACTION, "&Y$n appears in a spray of shooting light.", ch, NULL, NULL, TO_ROOM );
    send_to_char
        ( "\r\n&cYou wake up suddenly in a frightening place, with your head throbbing in pain!\r\nYou get to your feet, clutching your head in pain.\r\n\r\n",
          ch );
    set_position( ch, POS_STANDING );
    do_look( ch, ( char * ) "auto" );
    return;
}

// Gay command for wilderness exits, because I lack the coding skills to do this right - Vladaar
// Commented out on 4/1/09, Left here for Vladaar to know what exits to set up - Remcon
/*
void do_explore( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room;
    int                     rvnum,
                            ivnum;
    ROOM_INDEX_DATA        *to_room;

    if( ( ch->in_room->sector_type != SECT_AREA_ENT &&( ch->in_room->sector_type != SECT_CITY ) ) )
    {
        send_to_char( "\r\nThere is nothing to explore here.\r\n", ch );
        return;
    }

    ivnum = ch->in_room->vnum;

    if( !rvnum || !ivnum )
    {
    send_to_char("\r\nYou cannot explore that.\r\n", ch);
    bug( "%s: Do_explore from room %d.", __FUNCTION__, ch->in_room->vnum );
    return;
    }

    if( !room || !to_room  )
    {
    send_to_char("\r\nYou cannot explore that.\r\n", ch);
    bug( "%s: Do_explore from room %d.", __FUNCTION__, ch->in_room->vnum );
    return;
    }

    if( !ch->pcdata->htown )
    {
    bug( "%s: Do_explore player %s has no htown assigned.", __FUNCTION__, ch->name );
    }

    if( ivnum == 42844 )
        rvnum = 986;                                   // Citadel
    else if( ivnum == 42297 )
        rvnum = 4008;                                  // 'the garden'
    else if( ivnum == 4008 )
        rvnum = 42297;                                 // and back again! -- Volk
    else if( ivnum == 42408 )
        rvnum = 18631;                                 // Gnome Tower
    else if( ivnum == 18631 )
        rvnum = 42408;                                 // Leave Gnome Tower
    else if( ivnum == 42557 )
        rvnum = 11510;                                 // Apprentice Garden
    else if( ivnum == 11510 )
        rvnum = 42557;                                 // Leave Apprentice Garden
    else if( ivnum == 60792 )
        rvnum = 35601;                                 // School of Shadow
    else if( ivnum == 35601 )
        rvnum = 60792;                                 // Leave School of Shadow
    else if( ivnum == 62544 )
        rvnum = 2600;                                 // Enter Ancient Ruins
    else if( ivnum == 2600 )
        rvnum = 62544;                                 // Leave Ancient ruins
    else if( ivnum == 60640 )
        rvnum = 4851;                                 // Enter Mnera Temple
    else if( ivnum == 4851 )
        rvnum = 60640;                                // Leave Mnera Temple
    else if( ivnum == 60724 )
        rvnum = 2551;                                 // Enter Golden Penn
    else if( ivnum == 2551 )
        rvnum = 60724;                                 // Leave Golden Penn
    else if( ivnum == 42012 )
        rvnum = 31634;                                 // Enter Loramoor
    else if( ivnum == 31634 )
        rvnum = 42012;                                 // Leave Loramoor
    else if( ivnum == 41176 )
{
         if( !str_cmp(ch->pcdata->htown_name, "Paleon City" ))
        rvnum = 2904;                                 // Enter Shatkul
         if( !str_cmp(ch->pcdata->htown_name, "Dakar City" ))
        rvnum = 2920;
         if( !str_cmp(ch->pcdata->htown_name, "Forbidden City" ))
        rvnum = 2920;
}
    else if( ivnum == 2920 )                         // Leave Shatkul
        rvnum = 41176; 
    else if( ivnum == 2904 )
        rvnum = 41176;        
    else if( ivnum == 10800 )
        rvnum = 42120;                                 // Leave Jarynth
    else if( ivnum == 42120 )
        rvnum = 10800;                                 // Enter Jarynth
    else if( ivnum == 2300 )
        rvnum = 43842;                                 // Leave Underdark
    else if( ivnum == 43842 )
        rvnum = 2300;                                  // Enter Underdark
    else if( ivnum == 2758 )
        rvnum = 41195;                                 // Leave Merchant Discovery
    else if( ivnum == 41195 )
        rvnum = 2758;                                  // Enter Merchant Discovery
    else if( ivnum == 42300 )
        rvnum = 3503;
    else if( ivnum == 42040 )
        rvnum = 2500;                                 // Enter Foxden
    else if( ivnum == 2500 )
        rvnum = 42040;                                 // Leave Foxden
    else if( ivnum == 42884 )
        rvnum = 10001;                                 // Narcista Voyage
    else if( ivnum == 61108 )
        rvnum = 10201;                                 // Vila Island
    else if( ivnum == 43475 )
        rvnum = 10441;                                 // Isle of Death
    else if( ivnum == 43266 )
        rvnum = 10567;
    else if( ivnum == 42452 )
        rvnum = 16700;                                 // Cierra Mountain
    else if( ivnum == 42829 )
        rvnum = 18012;                                 // Regius Fonds
    else if( ivnum == 43436 )
        rvnum = 27023;                                 // kirwood
    else if( ivnum == 43446 )
        rvnum = 28251;
    else if( ivnum == 43948 )
        rvnum = 2200;                                  // Enter Szithlin
    else if( ivnum == 2200 )
        rvnum = 43948;                                 // Leave Szithlin
    else if( ivnum == 42168 )
        rvnum = 1850;                                  // Enter Ixious Village
    else if( ivnum == 1850 )
        rvnum = 42168;                                 // Leave Ixious Village
    else if( ivnum == 52319 )
        rvnum = 1750;                                  // Enter Floating Isle
    else if( ivnum == 1750 )
        rvnum = 52319;                                 // Leave Floating Isle
    else if( ivnum == 41458 )
        rvnum = 1400;                                  // Enter Crystal Forest
    else if( ivnum == 1400 )
        rvnum = 41458;                                 // Leave Cyrstal Forest
    else if( ivnum == 43916 )
        rvnum = 1650;                                  // Enter Desert Temple
    else if( ivnum == 1650 )
        rvnum = 43916;                                 // Leave Desert Temple
    else if( ivnum == 42152 )
        rvnum = 701;                                   // Enter Ice Castle
    else if( ivnum == 701 )
        rvnum = 42152;                                 // Leave Ice Castle
    else if( ivnum == 41683 )
        rvnum = 602;                                   // Enter Spider Swamp
    else if( ivnum == 602 )
        rvnum = 41683;                                 // Leave Spider Swamp
    else if( ivnum == 41428 )
        rvnum = 19500;                                 // Enter Estate of Unrest
    else if( ivnum == 19500 )
        rvnum = 41428;                                 // Leave Estate of Unrest
    else if( ivnum == 42050 )
        rvnum = 31300;
    else if( ivnum == 42081 )
        rvnum = 35500;                                 // Forbidden City entering
    else if( ivnum == 31300 )
        rvnum = 42050;
    else if( ivnum == 35500 )
        rvnum = 42081;                                 // Forbidden City leaving
    else if( ivnum == 3503 || ivnum == 16150 || ivnum == 16173 )
        rvnum = 42300;
    else if( ivnum == 51062 )
        rvnum = 31000;                                 // Entering Heaven
    else if( ivnum == 31000 )
        rvnum = 51062;                                 // Leaving Heaven
    else if( ivnum == 16700 )
        rvnum = 42452;
    else if( ivnum == 18012 )
        rvnum = 42829;                                 // Regius Fonds
    else if( ivnum == 986 )
        rvnum = 42844;                                 // Citadel
    else if( ivnum == 10001 )
        rvnum = 42884;
    else if( ivnum == 10567 )
        rvnum = 43266;
    else if( ivnum == 27023 )
        rvnum = 43436;
    else if( ivnum == 28251 )
        rvnum = 43446;                                 // Tufkular
    else if( ivnum == 10441 )
        rvnum = 43475;
    else if( ivnum == 10201 )
        rvnum =  61108;                                // Leaving Vila Island
    else if( ivnum == 41100 )
        rvnum = 18300;                                 // Durden
    else if( ivnum == 18300 )
        rvnum = 41100;
    else if( ivnum == 4951 )
        rvnum = 41769;                                 // Caverns of time back to
    // wilderness - one way!
    else
        return;

    to_room = get_room_index( rvnum );

    if( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area )
    {
        if( ch->level < to_room->area->low_hard_range )
        {
            set_char_color( AT_TELL, ch );
            switch( to_room->area->low_hard_range - ch->level )
            {
                case 1:
                    send_to_char( "A voice in your mind says, 'You are nearly ready to go that way...'\r\n", ch );
                    break;
                case 2:
                    send_to_char( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'\r\n", ch );
                    break;
                case 3:
                    send_to_char( "A voice in your mind says, 'You are not ready to go down that path... yet.'.\r\n", ch );
                    break;
                default:
                    send_to_char( "A voice in your mind says, 'You are not ready to go down that path.'.\r\n", ch );
            }
            return;
        }
        else if( ch->level > to_room->area->hi_hard_range )
        {
            set_char_color( AT_TELL, ch );
            send_to_char( "A voice in your mind says, 'There is nothing more for you down that path.'\r\n", ch );
            return;
        }
    }

    if( ( room = get_room_index( rvnum ) ) )
    {

        // Volk - quick custom message for caverns of time
        if( ivnum == 4951 )
        {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You feel a strange pulling sensation..\r\n", ch );
            set_char_color( AT_YELLOW, ch );
            send_to_char( "You are swallowed back into the normal passage of time!\r\n", ch );
            WAIT_STATE( ch, 6 );
        }

        EXIT_DATA              *xit = NULL;
        CREATE( xit, EXIT_DATA, 1 );

        xit->description = "somewhere";
        xit->keyword = "explore";
        xit->vdir = DIR_SOMEWHERE;
        xit->to_room = room;
        xit->vnum = rvnum;

        move_char( ch, xit, 0 );

        DISPOSE( xit );

    }
    else
        bug( "%s: No Room Index for room vnum %d.", __FUNCTION__, rvnum );

    return;
}
*/

void do_skip( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *sroom;
    int                     rvnum = 143;

    if ( ch->level > 1 ) {
        send_to_char( "You are too high level to skip the mud school.\r\n", ch );
        return;
    }
    if ( !( sroom = get_room_index( rvnum ) ) ) {
        send_to_char( "Can't find the room to send you to if you skip the mud school.\r\n", ch );
        bug( "%s: failed to find room vnum %d.", __FUNCTION__, rvnum );
        return;
    }
    act( AT_YELLOW, "You disappear in a spray of shooting light.", ch, NULL, ch, TO_CHAR );
    act( AT_YELLOW, "$n disappears in a spray of shooting light.", ch, NULL, ch, TO_ROOM );
    if ( xIS_SET( ch->act, PLR_TUTORIAL ) )
        xREMOVE_BIT( ch->act, PLR_TUTORIAL );
    char_from_room( ch );
    char_to_room( ch, sroom );
    act( AT_YELLOW, "$n appears in a spray of shooting light.", ch, NULL, ch, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
}

void do_expratio( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    short                   firstexpratio,
                            secondexpratio,
                            thirdexpratio;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    firstexpratio = atoi( arg1 );
    secondexpratio = atoi( arg2 );
    thirdexpratio = atoi( arg3 );

    if ( !IS_SECONDCLASS( ch ) ) {
        send_to_char( "You have no use for that command.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        if ( IS_THIRDCLASS( ch ) )
            ch_printf( ch, "Expratio Ratio: Class 1: %d Class 2: %d Class 3: %d \r\n",
                       ch->firstexpratio, ch->secondexpratio, ch->thirdexpratio );
        else
            ch_printf( ch, "Expratio Ratio: Class 1: %d Class 2: %d \r\n.", ch->firstexpratio,
                       ch->secondexpratio, ch->thirdexpratio );
    }

    if ( arg2[0] == '\0' || ( IS_THIRDCLASS( ch ) && arg3[0] == '\0' ) ) {
        if ( IS_THIRDCLASS( ch ) ) {
            send_to_char( "Triple classes - The three percents must add up to 100%.\r\n", ch );
            send_to_char( "Syntax: expratio % % %\r\n", ch );
        }
        else {
            send_to_char( "Dual classes - The two percents must add up to 100%.\r\n", ch );
            send_to_char( "Syntax: expratio % %\r\n", ch );
        }
        return;
    }

    if ( ( arg3[0] != '\0' ) && !IS_THIRDCLASS( ch ) ) {
        send_to_char( "You only have two classes, try again!\r\n", ch );
        return;
    }

    if ( firstexpratio < 0 || firstexpratio > 100 ) {
        send_to_char( "First class is out of range.\r\n", ch );
        return;
    }
    if ( secondexpratio < 0 || secondexpratio > 100 ) {
        send_to_char( "Second class is out of range.\r\n", ch );
        return;
    }
    if ( thirdexpratio < 0 || thirdexpratio > 100 ) {
        send_to_char( "Third class is out of range.\r\n", ch );
        return;
    }

    if ( !IS_THIRDCLASS( ch ) ) {                      /* Two classes */
        if ( firstexpratio + secondexpratio != 100 ) {
            send_to_char( "Dual classes - The two percents must add up to 100%.\r\n", ch );
            send_to_char( "Syntax: expratio % %\r\n", ch );
            return;
        }

        ch_printf( ch, "New exp ratio: Class 1: %d Class 2: %d\r\n", firstexpratio,
                   secondexpratio );

        ch->firstexpratio = firstexpratio;
        ch->secondexpratio = secondexpratio;
        return;
    }

    if ( firstexpratio + secondexpratio + thirdexpratio != 100 ) {
        send_to_char( "Triple classes - The three percents must add up to 100%.\r\n", ch );
        send_to_char( "Syntax: expratio % % %\r\n", ch );
        return;
    }

    ch_printf( ch, "New exp ratio: Class 1: %d Class 2: %d Class 3: %d\r\n", firstexpratio,
               secondexpratio, thirdexpratio );

    ch->firstexpratio = firstexpratio;
    ch->secondexpratio = secondexpratio;
    ch->thirdexpratio = thirdexpratio;
    return;
}

void do_ouch( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   chance;
    char                    buf[MAX_STRING_LENGTH];

    chance = number_range( 1, 3 );

    argument = one_argument( argument, arg );
    // Modified to find target if ch is fighting and can't see them. -Taon
    if ( arg[0] == '\0' ) {
        interpret( ch, ( char * ) "ooc OUCH!" );
        return;
    }
    else if ( ( victim = get_char_world( ch, arg ) ) != NULL ) {
        if ( chance == 1 ) {
            snprintf( buf, MSL, "[Ooc]: %s announces The STAFF of 6D send their condolences to %s.",
                      ch->name, victim->name );
            echo_to_all( AT_LBLUE, buf, ECHOTAR_ALL );
        }
        else if ( chance == 2 ) {
            snprintf( buf, MSL, "[Ooc]: %s sends flowers to %s next of kin.", ch->name,
                      victim->name );
            echo_to_all( AT_LBLUE, buf, ECHOTAR_ALL );
        }
        else if ( chance == 3 ) {
            snprintf( buf, MSL, "&W[&COoc&W]&C: %s comforts %s after their misfortune.", ch->name,
                      victim->name );
            echo_to_all( AT_LBLUE, buf, ECHOTAR_ALL );
        }
    }
    return;
}

void do_gratz( CHAR_DATA *ch, char *argument )
{
    short                   chance;

    if ( IS_NPC( ch ) )
        return;

    chance = number_range( 1, 19 );
    if ( chance == 1 )
        interpret( ch,
                   ( char * ) "ooc ,wonders if this could be the mythical &ZLEVELING MACHINE!" );
    else if ( chance == 2 )
        interpret( ch, ( char * ) "ooc &ZGRATZ!!!" );
    else if ( chance == 3 )
        interpret( ch, ( char * ) "ooc ,quickly hides the oil from the &ZLEVELING MACHINE!" );
    else if ( chance == 4 )
        interpret( ch, ( char * ) "ooc &ZHUZZAH!!!" );
    else if ( chance == 5 )
        interpret( ch, ( char * ) "ooc ,expects great things from that one!" );
    else if ( chance == 6 )
        interpret( ch, ( char * ) "ooc &ZWOOT!" );
    else if ( chance == 7 )
        interpret( ch, ( char * ) "ooc &ZGREAT JOB!" );
    else if ( chance == 8 )
        interpret( ch, ( char * ) "ooc &ZHUZZAH! &ZHUZZAH! &ZHUZZAH!" );
    else if ( chance == 9 )
        interpret( ch, ( char * ) "ooc &RC&YO&CN&OG&PR&WA&pT&cU&BL&RA&YT&CI&PO&WN&YS!!!&D" );
    else if ( chance == 10 )
        interpret( ch, ( char * ) "ooc &ZY&ZA&ZH&ZO&ZO&Zo&Zo&Zo&ZO&Zo&ZO&Zo!" );
    else if ( chance == 11 )
        interpret( ch, ( char * ) "ooc &ZWAY TO GOoOoO!" );
    else if ( chance == 12 )
        interpret( ch, ( char * ) "ooc OMG could it be....you must be a leveling &ZMACHINE!" );
    else if ( chance == 13 )
        interpret( ch, ( char * ) "ooc ,takes notice as another one rises in power!" );
    else if ( chance == 14 )
        interpret( ch, ( char * ) "ooc &ZWoot! Take a bow!" );
    else if ( chance == 15 )
        interpret( ch,
                   ( char * ) "ooc ,takes his hat off to you and your simply superb performance!" );
    else if ( chance == 16 )
        interpret( ch, ( char * ) "ooc &ZCongratulations, or as they say G'JOB!" );
    else if ( chance == 17 )
        interpret( ch,
                   ( char * )
                   "ooc ,can see by the fanfare and all the commotion...that somebody special just got a level!" );
    else if ( chance == 18 )
        interpret( ch, ( char * ) "ooc ,recognizes the Stunning performance!" );
    else if ( chance == 19 )
        interpret( ch,
                   ( char * )
                   "ooc &ZYou met the challenge with determination, strength, and total confidence! Congratulations!" );
}

void do_textspeed( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( !ch->pcdata )
        return;

    int                     number = atoi( argument );

    if ( number < 1 || number > 10 ) {
        send_to_char
            ( "Please use a number between 1 and 10 as your speed. Setting you to default (5).\r\n",
              ch );
        ch->pcdata->textspeed = 5;
        return;
    }

    ch->pcdata->textspeed = number;
    ch_printf( ch, "You have set your text speed to %d.\r\n", number );

    return;
}
