/**********************************************************************
 * Information: Future home of the upcoming arena system. -Taon       *
 *                                                                    *
 * Status: Nearing 74% completetion, code fully operational. Still    *
 * need to work out a few kinks, and finish establishing area types.  *
 *                                                                    * 
 * Code Information: The following code was written for 6dragons MUD, *
 * and remains the soul property of its Author(s). This header isn't  *
 * final but will do until I create a better one. -Taon               *
 * Other: This code is derived from our old arena code. All credits   *
 * to that author and other code contributors will be put here        *
 * as-well.                                                           *
 *                                                                    *
 *                                                                    *
 *                                                                    *
 **********************************************************************
 *                     Arena System Module                            *
 **********************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include "h/mud.h"
#include "h/clans.h"
#include "h/arena.h"
#include "h/hometowns.h"

void                    arena_enter( CHAR_DATA *ch );
void                    end_game( bool silent );
void                    establish_area( const char *argument );
void                    setup_type(  );
void                    find_arena_winner( CHAR_DATA *ch );
void                    parse_entry( short time );
void                    auto_tag( void );

bool                    arena_prep = FALSE;
bool                    arena_underway = FALSE;
bool                    challenge = FALSE;

short                   curr_game_time = 0;
short                   game_length;
short                   game_time_left;
short                   arena_type;

int                     arena_population = 0;
int                     arena_prep_vnum;
int                     max_level;
int                     start_time;
int                     time_left;

void do_cleararena( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( argument[0] == '\0' ) {
        send_to_char( "Clear arena bugs from whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL ) {
        send_to_char( "You are unable to find anyone by that name.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, PLR_BLUE ) )
        xREMOVE_BIT( victim->act, PLR_BLUE );
    if ( xIS_SET( victim->act, PLR_RED ) )
        xREMOVE_BIT( victim->act, PLR_RED );
    if ( xIS_SET( victim->act, PLR_PLAYING ) )
        xREMOVE_BIT( victim->act, PLR_PLAYING );
    if ( xIS_SET( victim->act, PLR_FROZEN ) )
        xREMOVE_BIT( victim->act, PLR_FROZEN );
    if ( xIS_SET( victim->act, PLR_TEASE ) )
        xREMOVE_BIT( victim->act, PLR_TEASE );
    if ( xIS_SET( victim->act, PLR_PKSAFE ) )
        xREMOVE_BIT( victim->act, PLR_PKSAFE );
    if ( xIS_SET( victim->act, PLR_WAITING ) )
        xREMOVE_BIT( victim->act, PLR_WAITING );

// make sure not still in arena
    if ( in_arena( victim ) ) {
        char_from_room( victim );
        char_to_room( victim, get_room_index( victim->pcdata->htown->recall ) );
    }

    send_to_char( "You have cleared them all arena bugs.\r\n", ch );
    return;
}

//Status: new command in testing phase. -Taon
void do_forfeit( CHAR_DATA *ch, char *argument )
{
    char                    buf[MIL];

    if ( IS_NPC( ch ) )
        return;

    // forfeit dragon tease
    if ( xIS_SET( ch->act, PLR_TEASE ) )
        xREMOVE_BIT( ch->act, PLR_TEASE );

    if ( !in_arena( ch ) ) {
        send_to_char( "But you're not in the arena.", ch );
        return;
    }

    stop_fighting( ch, TRUE );
    char_from_room( ch );
    char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );
    act( AT_TELL, "$n falls out of the sky!\r\n", ch, NULL, NULL, TO_ROOM );
    act( AT_TELL, "You fall suddenly out of the sky!\r\n", ch, NULL, NULL, TO_CHAR );
    ch->arena_loss++;
    snprintf( buf, MIL, "%s forfeits the arena!", ch->name );
    do_look( ch, ( char * ) "auto" );
    arena_chan( buf );
    --arena_population;

    if ( challenge ) {
        challenge = FALSE;
        end_game( FALSE );
    }
}

void do_arena( CHAR_DATA *ch, char *argument )
{
    char                    buf[MIL];
    char                    maxlvl[MIL];
    char                    area[MIL];
    char                    type[MIL];

    if ( IS_IMMORTAL( ch ) ) {
        argument = one_argument( argument, maxlvl );
        argument = one_argument( argument, area );
        argument = one_argument( argument, type );

        max_level = atoi( maxlvl );
        arena_type = atoi( type );

        if ( arena_prep || arena_underway ) {
            send_to_char( "The arena is already open!\r\n", ch );
            return;
        }
        if ( challenge ) {
            send_to_char( "There is currently a challenge taking place.\r\n", ch );
            return;
        }
        if ( !*maxlvl || !*area || !*type ) {
            send_to_char( "Syntax: arena <maxlevel> <area> <type>\r\n", ch );
            send_to_char( "Syntax: closearena\r\n", ch );
            send_to_char( "Areas available: jungle, desert, arctic, water\r\n", ch );
            send_to_char( "Types available: 1, 2, 3, 4\r\n", ch );
            send_to_char
                ( "1 = Player vs. Player, 2 = Freeze Tag, 3 = Dragon Tease, 4 = Random Arena\r\n",
                  ch );
            return;
        }
        if ( ( max_level || arena_type ) < 1 ) {
            send_to_char( "All numeric fields must contain values that are positive numbers.\r\n",
                          ch );
            return;
        }
        if ( arena_type == 2 ) {
            interpret( ch, ( char * ) "ftag start" );
            return;
        }
        if ( !strcmp( area, "jungle" ) || !strcmp( area, "desert" ) || !strcmp( area, "arctic" )
             || !strcmp( area, "water" ) )
            establish_area( area );
        else {
            send_to_char( "Invalid area entry.\r\n", ch );
            send_to_char( "Syntax: arena <maxlevel> <area> <type>\r\n", ch );
            send_to_char( "Areas available: jungle, desert, arctic, water\r\n", ch );
            return;
        }

        setup_type(  );

        arena_population = 0;
        arena_prep = TRUE;
        if ( arena_type == 3 ) {
            snprintf( buf, MIL, "%s has opened a level %d or below arena in the desert area!",
                      ch->name, max_level );
        }
        else {
            snprintf( buf, MIL, "%s has opened a level %d or below arena in the %s area!", ch->name,
                      max_level, area );
        }
        arena_chan( buf );
    }
    else {
        if ( arena_type == 2 ) {
            interpret( ch, ( char * ) "ftag join" );
            return;
        }

        if ( arena_prep )
            arena_enter( ch );
        else
            send_to_char( "The arena isn't currently allowing combatants.\r\n", ch );
    }
}

void do_closearena( CHAR_DATA *ch, char *argument )
{
    char                    buf[MIL];

    if ( arena_underway || arena_prep ) {
        snprintf( buf, MIL, "%s has closed the arena!", ch->name );
        arena_chan( buf );
        end_game( FALSE );
    }
    else
        send_to_char( "There isn't an arena currently running!\r\n", ch );
}

void arena_enter( CHAR_DATA *ch )
{
    char                    buf[MIL];

    if ( IS_NPC( ch ) )
        return;

    if ( ch->level > max_level ) {
        send_to_char( "Your level is above the current max level settings of the arena.\r\n \r\n",
                      ch );
        return;
    }
    if ( ch->level < 5 ) {
        send_to_char( "You're too young for the killing fields.", ch );
        return;
    }
    if ( !arena_prep ) {
        send_to_char( "The killing fields are currently closed.\r\n", ch );
        return;
    }
    if ( ch->in_room->vnum >= arena_low_vnum - 1 && ch->in_room->vnum <= arena_high_vnum + 1 ) {
        send_to_char( "But you're already in the arena.\r\n", ch );
        return;
    }
    act( AT_WHITE, "$n leaves the room for the arena.\r\n", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, get_room_index( arena_prep_vnum ) );
    act( AT_WHITE, "$n is dropped in from above.\r\n", ch, NULL, NULL, TO_ROOM );
    send_to_char( "You've joined the arena.\r\n", ch );
    do_look( ch, ( char * ) "auto" );
    arena_population++;
    ch->quest_curr += 5;
    ch->quest_accum += 5;
    ch_printf( ch, "Your glory has been increased by 5 for joining an arena event!\r\n" );

    if ( arena_type == 3 ) {
        snprintf( buf, MIL, "%s has joined the Dragon Tease Event! -[Won: %d / Lost: %d]-",
                  ch->name, ch->arena_wins, ch->arena_loss );
        arena_chan( buf );
    }
    if ( arena_type != 3 ) {
        snprintf( buf, MIL, "%s has joined the bloodbath! -[Won: %d / Lost: %d]-", ch->name,
                  ch->arena_wins, ch->arena_loss );
        arena_chan( buf );
    }
    if ( arena_type == 2 )
        ch->arena_mob_count = 0;
    else if ( arena_type == 3 )
        ch->arena_obj_count = 0;

    restore_char( ch );
}

CHAR_DATA              *greendragon = NULL;
OBJ_DATA               *dragonegg = NULL;

void run_game(  )
{
    CHAR_DATA              *player = NULL,
        *uplayer = NULL,
        *mob;
    ROOM_INDEX_DATA        *droom = NULL;
    DESCRIPTOR_DATA        *d;
    OBJ_DATA               *egg;
    char                    buf[MIL];
    bool                    dfound = FALSE;

    if ( get_arena_count(  ) <= 1 ) {
        snprintf( buf, MIL, "Match cancelled, not enough combatants!&D\r\n" );
        arena_chan( buf );
        end_game( FALSE );
        return;
    }

    for ( d = first_descriptor; d; d = d->next ) {
        if ( !d->character )
            continue;
        player = d->character;
        if ( !player->in_room || player->in_room->vnum != arena_prep_vnum )
            continue;

        if ( arena_type == 3 ) {
// stop players being changed to human priests - Vladaar
            if ( IS_AFFECTED( player, AFF_DRAGONLORD ) ) {
                interpret( player, ( char * ) "human" );
            }
            while ( player->first_affect )
                affect_remove( player, player->first_affect );
        }
        if ( player->position != POS_STANDING ) {
            set_position( player, POS_STANDING );
        }

        send_to_char
            ( "\r\nClick....\r\nSuddenly a trap door swings open from below, dropping you into the arena.\r\n",
              player );
        char_from_room( player );
        char_to_room( player, get_room_index( number_range( arena_low_vnum, arena_high_vnum ) ) );
        do_look( player, ( char * ) "auto" );
        do_visible( player, ( char * ) "" );
        if ( !uplayer || number_range( 1, 5 ) > 3 )
            uplayer = player;
        if ( arena_type == 3 )
            xSET_BIT( player->act, PLR_TEASE );
    }

    if ( arena_type == 3 ) {
        mob = create_mobile( get_mob_index( 41037 ) );
        greendragon = mob;

        while ( !droom || droom == uplayer->in_room )
            droom = get_room_index( number_range( arena_low_vnum, arena_high_vnum ) );

        /*
         * Random Room, that has no player in it 
         */
        char_to_room( mob, droom );

        if ( uplayer ) {
            egg = create_object( get_obj_index( 41007 ), 0 );
            dragonegg = egg;
            obj_to_char( egg, uplayer );
            send_to_char( "&RYou have stolen the dragon's egg!&D\r\n", uplayer );
        }
    }

    snprintf( buf, MIL, "The battle has begun with &O%d&C brave combatants!&D\r\n",
              arena_population );
    arena_chan( buf );
    arena_prep = FALSE;
    arena_underway = TRUE;
}

void end_game( bool silent )
{
    char                    buf[MIL];
    CHAR_DATA              *player;
    DESCRIPTOR_DATA        *d;

    for ( d = first_descriptor; d; d = d->next ) {
        if ( !d->character )
            continue;

        player = d->character;
        if ( !player->in_room || player->in_room->vnum < ( arena_low_vnum - 1 )
             || player->in_room->vnum > ( arena_high_vnum + 1 ) )
            continue;

        restore_char( player );
        if ( player->position == POS_STUNNED ) {
            player->position = POS_STANDING;
            update_pos( player );
        }
        player->challenged = NULL;
        player->challenge = NULL;
        stop_fighting( player, TRUE );
        do_clear( player, ( char * ) "" );
        if ( xIS_SET( player->act, PLR_TEASE ) )
            xREMOVE_BIT( player->act, PLR_TEASE );
        char_from_room( player );
        char_to_room( player, get_room_index( player->pcdata->htown->recall ) );
        act( AT_TELL, "$n falls out of the sky!\r\n", player, NULL, NULL, TO_ROOM );
// stop players being changed to human priests - Vladaar
        if ( IS_AFFECTED( player, AFF_DRAGONLORD ) ) {
            interpret( player, ( char * ) "human" );
        }
        while ( player->first_affect )
            affect_remove( player, player->first_affect );

        affect_strip( player, gsn_maim );
        affect_strip( player, gsn_brittle_bone );
        affect_strip( player, gsn_festering_wound );
        affect_strip( player, gsn_poison );
        affect_strip( player, gsn_thaitin );
        xREMOVE_BIT( player->affected_by, AFF_BRITTLE_BONES );
        xREMOVE_BIT( player->affected_by, AFF_FUNGAL_TOXIN );
        xREMOVE_BIT( player->affected_by, AFF_MAIM );
        xREMOVE_BIT( player->affected_by, AFF_THAITIN );

    }
    if ( arena_population > 1 && !silent ) {
        snprintf( buf, MIL, "After &O%d&C hours of battle the match is a draw.", curr_game_time );
        arena_chan( buf );
    }

    if ( greendragon ) {
        extract_char( greendragon, TRUE );
        greendragon = NULL;
    }

    if ( dragonegg ) {
        extract_obj( dragonegg );
        dragonegg = NULL;
    }

    arena_population = 0;
    arena_prep = FALSE;
    arena_underway = FALSE;
    challenge = FALSE;
    arena_low_vnum = 0;
    arena_high_vnum = 0;
    arena_prep_vnum = 0;
    curr_game_time = 0;
}

void silent_end(  )
{
    char                    buf[MAX_INPUT_LENGTH];

    if ( arena_population == 1 ) {
        sprintf( buf, "Not enough combatants, arena closing." );
        arena_chan( buf );
        end_game( TRUE );
        return;
    }

    arena_population = 0;
    arena_prep = FALSE;
    arena_underway = FALSE;
    challenge = FALSE;
    arena_low_vnum = 0;
    arena_high_vnum = 0;
    arena_prep_vnum = 0;
    curr_game_time = 0;

    sprintf( buf, "It looks like no one was brave enough to enter the Arena." );
    arena_chan( buf );
}

void arena_chan( char *argument )
{
    char                    bufx[MIL];
    DESCRIPTOR_DATA        *d;

    snprintf( bufx, MIL, "&RArena: &C%s&D\r\n", argument );
    for ( d = first_descriptor; d; d = d->next ) {
        if ( !d->connected && d->character )
            send_to_char( bufx, d->character );
    }
}

void establish_area( const char *argument )
{
    // check args to establish the proper information.
    // Setup proper hi-low vnums and prep area for arena.
    if ( arena_type == 3 ) {
        arena_low_vnum = 4401;
        arena_high_vnum = 4420;                        // Mera
        arena_prep_vnum = 4400;
        return;
    }

    if ( arena_type == 2 ) {
        arena_low_vnum = 5401;
        arena_high_vnum = 5420;                        // Narev
        arena_prep_vnum = 5400;
        return;
    }

    if ( !str_cmp( argument, "water" ) ) {
        arena_low_vnum = 4320;
        arena_high_vnum = 4339;                        // Vladaar
        arena_prep_vnum = 4340;
    }
    else if ( !str_cmp( argument, "arctic" ) ) {
        arena_low_vnum = 14500;
        arena_high_vnum = 14519;                       // Taon
        arena_prep_vnum = 14520;
    }
    else if ( !str_cmp( argument, "jungle" ) ) {
        arena_low_vnum = 5401;
        arena_high_vnum = 5420;                        // Narev
        arena_prep_vnum = 5400;
    }
    else if ( !str_cmp( argument, "desert" ) ) {
        arena_low_vnum = 4401;
        arena_high_vnum = 4420;                        // Mera
        arena_prep_vnum = 4400;
    }
    else
        bug( "%s: problem setting up the arena. Argument = [%s].", __FUNCTION__, argument );
}

void setup_type(  )
{
    char                    buf[MIL];

    // standard, player vs. player
    if ( arena_type == 1 ) {
        snprintf( buf, MIL, "                  &R<-[&WPlayer Vs. Player&R]->&d" );
        arena_chan( buf );
        game_length = 15;
    }
    else if ( arena_type == 2 ) {
        end_game( FALSE );
        snprintf( buf, MIL, "                  &R<-[&WFreeze Tag&R]->&d" );
        arena_chan( buf );
        auto_tag(  );
        return;
    }
    // Dragon Tease
    else if ( arena_type == 3 ) {
        snprintf( buf, MIL, "                  &R<-[&WDragon Tease&R]->&d" );
        arena_chan( buf );
        game_length = 25;
    }
    else {
        snprintf( buf, MIL, "                  &R<-[&WRandom Arena&R]->&d" );
        arena_chan( buf );
        game_length = number_chance( 15, 20 );
    }
    game_time_left = game_length;
    prep_length = 2;
}

void do_game( void )
{
    char                    buf[MIL];
    short                   chance;

    chance = number_chance( 1, 10 );

    if ( get_arena_count(  ) == 1 )
        find_arena_winner( NULL );
    else if ( game_time_left == 0 )
        end_game( FALSE );
    else {
        // Lets do some sanity checking here. -Taon
        if ( get_arena_count(  ) != arena_population ) {
            if ( get_arena_count(  ) > arena_population )
                log_printf( "Error: Arena has %d unidentified player(s) within the game.",
                            get_arena_count(  ) - arena_population );
            else
                log_printf( "Error: Arena is missing %d player(s) from the game.",
                            arena_population - get_arena_count(  ) );
            log_string( "Error: Adjusting arena population." );
            arena_population = get_arena_count(  );
        }
        if ( chance >= 9 ) {
            snprintf( buf, MIL, "%d contestants remain inside of the arena!\r\n",
                      arena_population );
            arena_chan( buf );
        }
    }
    game_time_left--;
    curr_game_time++;
}

//This is unfinished, about 95% completed. -Taon
void do_challenge( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    buf[MIL];

    argument = one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( arg[0] == '\0' ) {
        send_to_char( "You must provide a target to challenge!\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "withdraw" ) && ch->challenge ) {
        snprintf( buf, MIL, "%s withdraws the challenge to duel.\r\n", ch->name );
        arena_chan( buf );
        ch->challenge = NULL;
        challenge = FALSE;
        return;
    }
    if ( arena_prep || arena_underway ) {
        send_to_char( "There is already an arena underway!\r\n", ch );
        return;
    }
    if ( challenge ) {
        send_to_char( "There is currently another challenge taking place!\r\n", ch );
        return;
    }
    if ( ch->level < 6 ) {
        send_to_char( "Your level isn't high enough to challenge anyone!\r\n", ch );
        return;
    }
    if ( !IS_AWAKE( ch ) ) {
        send_to_char( "But you're sleeping!\r\n", ch );
        return;
    }

    victim = get_char_world( ch, arg );

    if ( !victim ) {
        send_to_char( "They're currently not logged in.\r\n", ch );
        return;
    }

/*  Dont put a message you can't challenge that player here.  It gives away puppets.  Find a diff solution  Vladaar
  if(IS_PUPPET(victim) || IS_PUPPET(ch))
  {
  }
*/

    if ( victim == ch ) {
        send_to_char( "Challenge yourself?!?", ch );
        return;
    }
    if ( IS_IMMORTAL( ch ) || IS_IMMORTAL( victim ) ) {
        send_to_char( "Staff dont have any place in the arenas.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You cannot challenge mobs to an arena match.\r\n", ch );
        return;
    }
    if ( victim->level < 6 ) {
        send_to_char
            ( "You should be ashamed of yourself for attempting to fight such a weak foe.\r\n",
              ch );
        return;
    }
    if ( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) ) {
        send_to_char( "They would beat you to a bloody pulp!\r\n", ch );
        return;
    }

    challenge = TRUE;
    victim->challenged = ch;
    ch->challenge = victim;
    snprintf( buf, MIL, "%s has challenged %s to a duel.\r\n", ch->name, victim->name );
    arena_chan( buf );
    send_to_char( "&CYou can either &WACCEPT&C or &WDECLINE&C this challenge!\r\n", victim );
    return;
}

void do_decline( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *challenger;
    char                    buf[MIL];

    if ( !challenge ) {
        send_to_char( "No Challenges are taking place.", ch );
        return;
    }

    if ( !ch->challenged ) {
        send_to_char( "But you're not being challenged!\r\n", ch );
        return;
    }

    challenger = ch->challenged;
    snprintf( buf, MIL, "%s declines %s's challenge!", ch->name, challenger->name );
    arena_chan( buf );

    challenger->challenge = NULL;
    ch->challenged = NULL;
    challenge = FALSE;
}

void do_accept( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *challenger;
    ROOM_INDEX_DATA        *preproom;
    char                    buf[MIL];
    char                    arg[MIL];

    if ( !challenge ) {
        send_to_char( "No Challenges are taking place.", ch );
        return;
    }

    if ( ch->challenged == NULL ) {
        send_to_char( "You haven't been issued a challenge!\r\n", ch );
        return;
    }
    arena_prep = TRUE;
    prep_length = 4;
    challenger = ch->challenged;
    game_time_left = 20;
    argument = one_argument( argument, arg );
    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Syntax: Accept <arena type>\r\n", ch );
        send_to_char( "Arena Types available include: water, arctic, jungle, desert\r\n", ch );
        return;
    }
    arena_type = 1;
    if ( !str_cmp( arg, "water" ) )
        establish_area( "water" );
    else if ( !str_cmp( arg, "arctic" ) )
        establish_area( "arctic" );
    else if ( !str_cmp( arg, "jungle" ) )
        establish_area( "jungle" );
    else if ( !str_cmp( arg, "desert" ) )
        establish_area( "desert" );
    else {                                             // default?
        send_to_char( "\r\nThere is no arena type named that, please try again.\r\n", ch );
        send_to_char( "Syntax: Accept <arena type>\r\n", ch );
        send_to_char( "Arena Types available include: water, arctic, jungle, desert\r\n", ch );
        return;
    }

    if ( !( preproom = get_room_index( arena_prep_vnum ) ) ) {
        bug( "%s: Couldn't find room %d for the dual to take place.", __FUNCTION__,
             arena_prep_vnum );
        send_to_char
            ( "There was a problem in creating the rooms so not going to continue with the dual.\r\n",
              ch );
        send_to_char
            ( "There was a problem in creating the rooms so not going to continue with the dual.\r\n",
              challenger );
        arena_population = 0;
        ch->challenged = NULL;
        challenger->challenge = NULL;
        return;
    }
    char_from_room( ch );
    char_to_room( ch, get_room_index( arena_prep_vnum ) );
    do_look( ch, ( char * ) "auto" );
    char_from_room( challenger );
    char_to_room( challenger, get_room_index( arena_prep_vnum ) );
    do_look( challenger, ( char * ) "auto" );
    arena_population = 2;
    ch->challenged = NULL;
    challenger->challenge = NULL;

    snprintf( buf, MIL, "%s accepts %s's challenge!.", ch->name, challenger->name );
    arena_chan( buf );
}

void find_arena_winner( CHAR_DATA *ch )
{
    CHAR_DATA              *player;
    CLAN_DATA              *clan = NULL;
    DESCRIPTOR_DATA        *descr;
    char                    buf[MIL];

    for ( descr = first_descriptor; descr; descr = descr->next ) {
        if ( !descr->connected ) {
            player = descr->original ? descr->original : descr->character;

            if ( player->in_room->vnum >= arena_low_vnum - 1
                 && ( player->in_room->vnum <= arena_high_vnum + 1 ) ) {
                restore_char( player );

                // Lets track clan wins in arenas. -Taon
                if ( !IS_NPC( player ) ) {
                    if ( ch && ch->pcdata && ch->pcdata->clan )
                        clan = ch->pcdata->clan;
                    else if ( player->pcdata->clan )
                        clan = player->pcdata->clan;
                    if ( clan )
                        clan->arena_victory++;
                }
                if ( !ch ) {
                    snprintf( buf, MIL, "%s is declared the winner!", player->name );
                    player->arena_wins++;
                    if ( !challenge ) {
                        player->quest_curr += 25;
                        player->quest_accum += 25;
                        ch_printf( player,
                                   "Your glory has been increased by 25 for winning the arena event!\r\n" );
                    }
                    if ( xIS_SET( player->act, PLR_TEASE ) )
                        xREMOVE_BIT( player->act, PLR_TEASE );
                }
                else {
                    snprintf( buf, MIL, "%s is declared the winner!", ch->name );
                    ch->arena_wins++;
                    if ( !challenge ) {
                        ch->quest_curr += 25;
                        ch->quest_accum += 25;
                        ch_printf( ch,
                                   "Your glory has been increased by 25 for winning the arena event!\r\n" );
                    }
                    if ( xIS_SET( ch->act, PLR_TEASE ) )
                        xREMOVE_BIT( ch->act, PLR_TEASE );
                }
                arena_chan( buf );
                end_game( TRUE );
            }
        }
    }
}

void load_arena_mob(  )
{
    return;
}

void parse_entry( short time )
{
    char                    buf[MIL];

    if ( time >= 1 && !challenge ) {
        snprintf( buf, MIL,
                  "Use the &WARENA&C command to enter the arena. Only %d hour(s) until chaos begins.",
                  time );
        arena_chan( buf );
    }
}

short get_arena_count(  )
{
    CHAR_DATA              *player;
    DESCRIPTOR_DATA        *d;
    short                   count = 0;

    for ( d = first_descriptor; d; d = d->next )
        if ( !d->connected ) {
            player = d->original ? d->original : d->character;
            if ( player->in_room->vnum >= arena_low_vnum - 1
                 && ( player->in_room->vnum <= arena_high_vnum + 1 ) )
                count++;
        }
    return count;
}

// Aglance Code from Age of Heroes by Xraksis

void do_aglance( CHAR_DATA *ch, char *argument )
{
    char                    buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA        *d;
    int                     hp_stats = 0;
    int                     mana_stats = 0;
    int                     move_stats = 0;

    if ( IS_NPC( ch ) )
        return;
    if ( ch->in_room != NULL ) {
        if ( IS_SET( ch->in_room->room_flags, ROOM_ARENA )
             && str_cmp( ch->in_room->area->filename, "throngfort.are" )
             && str_cmp( ch->in_room->area->filename, "allifort.are" ) ) {
            send_to_char( "You're in the arena.\n\r", ch );
            return;
        }
    }
    send_to_char( "&c            People in the Arena\n\r\n\r", ch );
    send_to_char( "Name               Hp        Move       Mana\n\r", ch );
    send_to_char( "----------------------------------------------&C\n\r", ch );
    for ( d = first_descriptor; d != NULL; d = d->next ) {
        if ( d->character != NULL ) {
            if ( d->character->in_room != NULL ) {
                if ( d->character->in_room
                     && !str_cmp( d->character->in_room->area->filename, "throngfort.are" )
                     || !str_cmp( d->character->in_room->area->filename, "allifort.are" ) )
                    continue;
                if ( !IS_SET( d->character->in_room->room_flags, ROOM_ARENA ) )
                    continue;
                if ( d->character->max_hit > 0 )
                    hp_stats = 100 * d->character->hit / d->character->max_hit;
                if ( d->character->max_move > 0 )
                    move_stats = 100 * d->character->move / d->character->max_move;
                if ( d->character->max_mana > 0 )
                    mana_stats = 100 * d->character->mana / d->character->max_mana;
                sprintf( buf, "%-15s    %3d/100   %3d/100   %3d/100\n\r", d->character->name,
                         hp_stats, move_stats, mana_stats );
                send_to_char( buf, ch );
            }
        }
    }
    return;
}
