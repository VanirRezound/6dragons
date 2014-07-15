/* 6 Dragons - teacher code 1/1/2003 */

#include "h/mud.h"
#include <string.h>

#define CANT_PRAC "Tongue"

CHAR_DATA              *get_teacher( int vnum )
{
    CHAR_DATA              *wch;

    if ( !get_mob_index( vnum ) )
        return NULL;

    /*
     * check the world for an exact match 
     */
    for ( wch = first_char; wch; wch = wch->next )
        if ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum )
            if ( IS_NPC( wch )
                 && ( xIS_SET( wch->act, ACT_PRACTICE ) || xIS_SET( wch->act, ACT_NEWBPRACTICE ) ) )
                return wch;
    return NULL;
}

bool can_teach( CHAR_DATA *ch, CHAR_DATA *teacher, int sn )
{
    char                    buf[MSL];
    bool                    newpract = FALSE,
        isteacher = FALSE;

    if ( sn < 0 || sn > top_sn ) {
        bug( "%s: invalid sn [%d]", __FUNCTION__, sn );
        return FALSE;
    }

    if ( !ch || !teacher ) {
        bug( "%s: NULL ch/teacher [%d]", __FUNCTION__, sn );
        return FALSE;
    }

    /*
     * If not an NPC can't teach anything 
     */
    if ( !IS_NPC( teacher ) )
        return FALSE;

    /*
     * If not a practice or a newbie practice can't teach anything 
     */
    if ( !xIS_SET( teacher->act, ACT_PRACTICE ) && !xIS_SET( teacher->act, ACT_NEWBPRACTICE ) )
        return FALSE;

    /*
     * Only a newbie practice teacher 
     */
    if ( !xIS_SET( teacher->act, ACT_PRACTICE ) && xIS_SET( teacher->act, ACT_NEWBPRACTICE ) )
        newpract = TRUE;

    if ( VLD_STR( skill_table[sn]->teachers ) ) {
        snprintf( buf, MSL, "%d", teacher->pIndexData->vnum );
        if ( is_name( buf, skill_table[sn]->teachers ) )
            isteacher = TRUE;
    }

    if ( IS_THIRDCLASS( ch ) ) {
        if ( !newpract || skill_table[sn]->skill_level[ch->thirdclass] <= 20 || isteacher ) {
            if ( ch->thirdlevel >= skill_table[sn]->skill_level[ch->thirdclass] )
                return TRUE;
        }
        if ( !newpract || skill_table[sn]->skill_level[ch->secondclass] <= 20 || isteacher ) {
            if ( ch->secondlevel >= skill_table[sn]->skill_level[ch->secondclass] )
                return TRUE;
        }

        if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 || isteacher ) {
            if ( ch->firstlevel >= skill_table[sn]->skill_level[ch->Class] )
                return TRUE;
        }
    }
    else if ( IS_SECONDCLASS( ch ) ) {
        if ( !newpract || skill_table[sn]->skill_level[ch->secondclass] <= 20 || isteacher ) {
            if ( ch->secondlevel >= skill_table[sn]->skill_level[ch->secondclass] )
                return TRUE;
        }

        if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 || isteacher ) {
            if ( ch->firstlevel >= skill_table[sn]->skill_level[ch->Class] )
                return TRUE;
        }
    }
    else if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 || isteacher ) {
        if ( ch->level >= skill_table[sn]->skill_level[ch->Class] )
            return TRUE;
    }

    return FALSE;
}

bool can_show_teach( CHAR_DATA *ch, CHAR_DATA *teacher, int sn )
{
    char                    buf[MSL];
    bool                    newpract = FALSE;

    if ( sn < 0 || sn > top_sn ) {
        bug( "%s: invalid sn [%d]", __FUNCTION__, sn );
        return FALSE;
    }

    if ( !ch || !teacher ) {
        bug( "%s: NULL ch/teacher [%d]", __FUNCTION__, sn );
        return FALSE;
    }

    /*
     * If not an NPC can't teach anything
     */
    if ( !IS_NPC( teacher ) )
        return FALSE;

    /*
     * If not a practice or a newbie practice can't teach anything
     */
    if ( !xIS_SET( teacher->act, ACT_PRACTICE ) && !xIS_SET( teacher->act, ACT_NEWBPRACTICE ) )
        return FALSE;

    /*
     * Only a newbie practice teacher
     */
    if ( !xIS_SET( teacher->act, ACT_PRACTICE ) && xIS_SET( teacher->act, ACT_NEWBPRACTICE ) )
        newpract = TRUE;

    if ( VLD_STR( skill_table[sn]->teachers ) ) {
        snprintf( buf, MSL, "%d", teacher->pIndexData->vnum );
        if ( !is_name( buf, skill_table[sn]->teachers ) )
            return FALSE;
        else
            return TRUE;                               /* If they are a teacher of it,
                                                        * show it */
    }

    if ( IS_THIRDCLASS( ch ) ) {
        if ( !newpract || skill_table[sn]->skill_level[ch->thirdclass] <= 20 )
            return TRUE;
        if ( !newpract || skill_table[sn]->skill_level[ch->secondclass] <= 20 )
            return TRUE;
        if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 )
            return TRUE;
    }
    else if ( IS_SECONDCLASS( ch ) ) {
        if ( !newpract || skill_table[sn]->skill_level[ch->secondclass] <= 20 )
            return TRUE;
        if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 )
            return TRUE;
    }
    else if ( !newpract || skill_table[sn]->skill_level[ch->Class] <= 20 )
        return TRUE;

    return FALSE;
}

void do_teacher( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    short                   lasttype = SKILL_SPELL,
        cnt = 0;
    int                     sn,
                            col = 0;
    char                    buf[MSL];

    for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
        if ( IS_NPC( mob )
             && ( xIS_SET( mob->act, ACT_PRACTICE ) || xIS_SET( mob->act, ACT_NEWBPRACTICE ) ) )
            break;
    if ( !mob ) {
        send_to_char( "There is no teacher here.\r\n", ch );
        return;
    }

    if ( xIS_SET( mob->act, ACT_NEWBPRACTICE ) ) {
        act( AT_TELL, "\r\n&W$n tells you 'I can teach any spell/skill under level 20.", mob, NULL,
             ch, TO_VICT );
        send_to_char( "&WAfter level 20, you will have to seek out teachers.'\r\n", ch );
        return;
    }
    act( AT_TELL, "\r\n$n tells you 'I can teach the following:'\r\n", mob, NULL, ch, TO_VICT );

    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        if ( !skill_table[sn]->teachers )
            continue;

        if ( !can_show_teach( ch, mob, sn ) )          /* If they can't teach it no point 
                                                        * in showing it */
            continue;

        if ( strcmp( skill_table[sn]->name, "reserved" ) == 0 && !IS_NPC( ch ) ) {
            if ( col != 0 ) {
                send_to_pager( "\r\n", ch );
                col = 0;
            }
            set_pager_color( AT_LBLUE, ch );
            send_to_pager_color( "&C                               _.:[", ch );
            set_pager_color( AT_CYAN, ch );
            send_to_pager_color( "&c Magicks ", ch );
            set_pager_color( AT_LBLUE, ch );
            send_to_pager_color( "&C]:._\r\n", ch );
        }

        if ( skill_table[sn]->type != lasttype ) {
            int                     count,
                                    type_length;
            char                    space_buf[MSL];

            if ( !cnt )
                send_to_pager( "                                   (none)\r\n", ch );
            else if ( col != 0 ) {
                send_to_pager( "\r\n", ch );
                col = 0;
            }
            space_buf[0] = '\0';
            type_length = ( ( 79 - ( strlen( skill_tname[skill_table[sn]->type] ) + 11 ) ) / 2 );
            for ( count = 0; count < type_length; count++ )
                mudstrlcat( space_buf, " ", MSL );
            set_pager_color( AT_CYAN, ch );
            pager_printf_color( ch, "%s_.:[", space_buf );
            set_pager_color( AT_CYAN, ch );
            pager_printf_color( ch, " &c%ss&C ",
                                !str_cmp( skill_tname[skill_table[sn]->type],
                                          "skill" ) ? "Abilitie" : skill_tname[skill_table[sn]->
                                                                               type] );
            set_pager_color( AT_CYAN, ch );
            send_to_pager_color( "]:._\r\n", ch );
            cnt = 0;
        }

        lasttype = skill_table[sn]->type;
        if ( VLD_STR( skill_table[sn]->teachers ) ) {
            snprintf( buf, MSL, "%d", mob->pIndexData->vnum );
            if ( !is_name( buf, skill_table[sn]->teachers ) )
                continue;
        }
        ++cnt;
        set_pager_color( AT_CYAN, ch );
        pager_printf( ch, "&c%25.25s&C", skill_table[sn]->name );
        if ( ++col == 3 ) {
            send_to_pager( "\r\n", ch );
            col = 0;
        }
    }
    if ( col != 0 )
        send_to_pager( "\r\n", ch );
}

/* Show all teachers */
void do_listteachers( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *wch;

    if ( !ch || IS_NPC( ch ) || ch->level < LEVEL_IMMORTAL )
        return;
    /*
     * check the world for an exact match 
     */
    send_to_char( "&CThe current list of teachers and locations in 6 Dragons&D\r\n\r\n", ch );
    send_to_char( "&rTeacher name                  Teacher Vnum        Teacher Room Location\r\n",
                  ch );
    send_to_char( "&R-----------------------------------------------------------------------\r\n",
                  ch );
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( IS_NPC( wch ) && ( xIS_SET( wch->act, ACT_PRACTICE ) ) )
            ch_printf( ch, "%-35s          %-8d        %-8d.\r\n",
                       VLD_STR( wch->pIndexData->short_descr ) ? wch->pIndexData->
                       short_descr : "No short_descr set", wch->pIndexData->vnum,
                       wch->in_room->vnum );
    }
}

void do_listscholar( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *wch;

    if ( !ch || IS_NPC( ch ) || ch->level < LEVEL_IMMORTAL )
        return;
    /*
     * check the world for an exact match
     */
    send_to_char( "&CThe current list of scholars and locations in 6 Dragons&D\r\n\r\n", ch );
    send_to_char( "&rScholar name                  Scholar Vnum        Scholar Room Location\r\n",
                  ch );
    send_to_char( "&R-----------------------------------------------------------------------\r\n",
                  ch );
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( IS_NPC( wch ) && ( xIS_SET( wch->act, ACT_SCHOLAR ) ) )
            ch_printf( ch, "%-35s          %-8d        %-8d.\r\n",
                       VLD_STR( wch->pIndexData->short_descr ) ? wch->pIndexData->
                       short_descr : "No short_descr set", wch->pIndexData->vnum,
                       wch->in_room->vnum );
    }
}
