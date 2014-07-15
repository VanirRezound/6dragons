/*	Volk  trying to add automapper function!  */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "h/mud.h"

void                    draw_map( CHAR_DATA *ch, unsigned short int map[50][50] );

void                    CheckRoom( CHAR_DATA *ch, ROOM_INDEX_DATA *location, int PosX, int PosY,
                                   unsigned short int map[50][50] );

void                    CheckExitDir( CHAR_DATA *ch, ROOM_INDEX_DATA *location, int PosX, int PosY,
                                      int Exd, unsigned short int map[50][50] );

int                     centerx,
                        centery,
                        MapWidth,
                        MapLength;

char                   *you_are_here( int row, int col, char *map );

void do_lookmap( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *location;
    int                     x,
                            y;
    unsigned short int      map[50][50];
    short                   maplag = 0;

    if ( !str_cmp( argument, "auto" ) ) {
        if ( IN_WILDERNESS( ch ) ) {
            if ( ch->map_size == 0 ) {
                MapLength = 10;
                MapWidth = 10;
            }
            else if ( ch->map_size == 1 ) {
                MapLength = 12;
                MapWidth = 12;
            }
            else if ( ch->map_size == 2 ) {
                MapLength = 15;
                MapWidth = 15;
            }
            else {
                MapLength = 6;
                MapWidth = 6;
            }
        }
    }
    else {
        MapWidth = atoi( argument );
        MapLength = atoi( argument );
        maplag = MapWidth / 2;
    }

    if ( MapWidth > 15 && !IS_IMMORTAL( ch ) ) {
        if ( !IS_AFFECTED( ch, AFF_WIZARD_SIGHT ) ) {
            send_to_char( "&BYou can only view a small portion of the area around you.&w\r\n", ch );
            MapWidth = 15;
        }
        else
            send_to_char( "&BUsing your Wizard Sight you are able to view places far away..&w\r\n",
                          ch );
        maplag = MapWidth / 5;
    }

    /*
     * Keep the diamond one set size 
     */
    if ( ch->map_type == 1 ) {
        MapLength = 15;
        MapWidth = 29;
    }

    if ( ch->map_type == 0 && ( IS_AFFECTED( ch, AFF_BATTLEFIELD ) || IS_IMMORTAL( ch ) ) ) {
        MapLength = 15;
        MapWidth = 50;
    }

    /*
     * Keep the sphere one set size 
     */
    if ( ch->map_type == 2 ) {
        MapLength = 15;
        MapWidth = 29;
    }

    if ( MapWidth < 3 )
        MapWidth = 3;
    else if ( MapWidth > 49 )
        MapWidth = 49;

    if ( MapLength < 3 )
        MapLength = 3;
    else if ( MapLength > 49 )
        MapLength = 49;

    x = ( MapWidth / 2 ) * 2;
    if ( MapWidth == x )
        MapWidth--;
    centerx = MapWidth / 2 + 1;

    x = ( MapLength / 2 ) * 2;
    if ( MapLength == x )
        MapLength--;
    centery = MapLength / 2 + 1;

    location = get_room_index( ch->in_room->vnum );
    for ( y = 0; y <= MapLength; ++y ) {
        for ( x = 0; x <= MapWidth; ++x )
            map[x][y] = ( SECT_MAX + 1 );
    }
    CheckRoom( ch, location, centerx, centery, map );
    map[centerx][centery] = ch->in_room->sector_type;
    draw_map( ch, map );
}

void CheckRoom( CHAR_DATA *ch, ROOM_INDEX_DATA *location, int CX, int CY,
                unsigned short int map[50][50] )
{
    int                     PosX,
                            PosY;

    PosX = CX;
    PosY = CY - 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_NORTH, map );

    PosX = CX;
    PosY = CY + 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_SOUTH, map );

    PosX = CX + 1;
    PosY = CY;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_EAST, map );

    PosX = CX - 1;
    PosY = CY;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_WEST, map );

    PosX = CX + 1;
    PosY = CY - 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_NORTHEAST, map );

    PosX = CX - 1;
    PosY = CY - 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_NORTHWEST, map );

    PosX = CX + 1;
    PosY = CY + 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_SOUTHEAST, map );

    PosX = CX - 1;
    PosY = CY + 1;
    if ( PosX <= MapWidth && PosY <= MapLength && PosX > 0 && PosY > 0 )
        CheckExitDir( ch, location, PosX, PosY, DIR_SOUTHWEST, map );

    /*
     * Set the right information here if it's an explore room 
     */
    if ( location->sector_type == SECT_AREA_ENT ) {
        EXIT_DATA              *xit;
        ROOM_INDEX_DATA        *elocation;

        if ( ( xit = get_exit( location, DIR_EXPLORE ) ) ) {
            if ( ( elocation = get_room_index( xit->vnum ) ) ) {
                if ( !IS_IMMORTAL( ch )
                     && ( elocation->area->low_hard_range > ch->level
                          || elocation->area->hi_hard_range < ch->level ) )
                    map[CX][CY] = ( SECT_MAX + 5 );    // RED
                else if ( elocation->area->low_soft_range <= ( ch->level - 4 )
                          || IS_IMMORTAL( ch ) )
                    map[CX][CY] = ( SECT_MAX + 2 );    // GRAY
                else if ( elocation->area->low_soft_range >= ( ch->level + 4 )
                          && ( elocation->area->low_hard_range < ch->level ) )
                    map[CX][CY] = ( SECT_MAX + 4 );    // YELLOW
                else if ( elocation->area->low_soft_range >= ( ch->level - 3 )
                          && elocation->area->low_soft_range <= ( ch->level + 3 ) )
                    map[CX][CY] = ( SECT_MAX + 3 );    // GREEN
            }
        }
    }
    else {
        if ( location->first_person ) {
            CHAR_DATA              *rch;
            int                     num = 0;

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( rch == ch )
                    continue;

                if ( !IS_NPC( rch ) && can_see( ch, rch ) ) {
                    num++;
                    break;
                }
            }

            if ( num > 0 )
                map[CX][CY] = ( SECT_MAX + 6 );
        }
    }
}

void CheckExitDir( CHAR_DATA *ch, ROOM_INDEX_DATA *location, int X, int Y, int ExD,
                   unsigned short int map[50][50] )
{
    EXIT_DATA              *xit,
                           *cxit;
    int                     olvnum;

    olvnum = location->vnum;                           /* Need this to check vnums and
                                                        * make sure reverse exit leads
                                                        * back here */

    if ( ( xit = get_exit( location, ExD ) ) ) {
        if ( map[X][Y] == ( SECT_MAX + 1 ) && !IS_SET( xit->exit_info, EX_CLOSED ) ) {
            location = get_room_index( xit->vnum );

            /*
             * Lets see if we have a reverse exit if the reverse doesn't exist or point back
             * then give a bug message 
             */
            if ( !( cxit = get_exit( location, rev_dir[ExD] ) ) || cxit->vnum != olvnum )
                bug( "%s: Room %d exit %d should point to Room %d but doesn't.", __FUNCTION__,
                     location->vnum, rev_dir[ExD], olvnum );

            if ( location->sector_type )
                map[X][Y] = location->sector_type;
            else
                map[X][Y] = SECT_INSIDE;
            CheckRoom( ch, location, X, Y, map );
        }
    }
}

char                   *map_type( int map_position )
{
    const char             *sect;

    switch ( map_position ) {
        default:
            sect = "&R?";
            break;
        case SECT_INSIDE:
            sect = "&z#";
            break;
        case SECT_ROAD:
            sect = "&P.";
            break;
        case SECT_JUNGLE:
            sect = "&g&&";
            break;
        case SECT_VROAD:
            sect = "&O|";
            break;
        case SECT_HROAD:
            sect = "&O-";
            break;
        case SECT_CAMPSITE:
            sect = "&Rx";
            break;
        case SECT_DOCK:
            sect = "&w0";
            break;
        case SECT_LAKE:
            sect = "&CO";
            break;
        case SECT_ARCTIC:
            sect = "&W*";
            break;
        case SECT_CROSSROAD:
            sect = "&Oo";
            break;
        case SECT_THICKFOREST:
            sect = "&gT";
            break;
        case SECT_HIGHMOUNTAIN:
            sect = "&W^^";
            break;
        case SECT_GRASSLAND:
            sect = "&g'";
            break;
        case SECT_CITY:
            sect = "&W@";
            break;
        case SECT_FIELD:
            sect = "&G=";
            break;
        case SECT_FOREST:
            sect = "&GT";
            break;
        case SECT_HILLS:
            sect = "&g^^";
            break;
        case SECT_MOUNTAIN:
            sect = "&w^^";
            break;
        case SECT_WATER_SWIM:
            sect = "&C~";
            break;
        case SECT_SWAMP:
            sect = "&G%";
            break;
        case SECT_AREA_ENT:
            sect = "&Y@";
            break;
        case SECT_WATERFALL:
            sect = "&cW";
            break;
        case SECT_RIVER:
            sect = "&B:";
            break;
        case SECT_OCEAN:
        case SECT_WATER_NOSWIM:
            sect = "&B~";
            break;
        case SECT_UNDERWATER:
            sect = "&B-";
            break;
        case SECT_AIR:
            sect = "&W@";
            break;
        case SECT_DESERT:
            sect = "&Y=";
            break;
        case SECT_OCEANFLOOR:
            sect = "&b-";
            break;
        case SECT_UNDERGROUND:
            sect = "&z.";
            break;
        case SECT_LAVA:
            sect = "&R~";
            break;
        case SECT_PORTALSTONE:
            sect = "&wP";
            break;
        case SECT_DEEPMUD:
            sect = "&Ou";
            break;
        case SECT_QUICKSAND:
            sect = "&OU";
            break;
        case SECT_PASTURELAND:
            sect = "&Gm";
            break;
        case SECT_VALLEY:
            sect = "&gV";
            break;
        case SECT_MOUNTAINPASS:
            sect = "&Ov";
            break;
        case SECT_BEACH:
            sect = "&Y:";
            break;
        case SECT_FOG:
            sect = "&wF";
            break;
        case SECT_SKY:
            sect = "&C@";
            break;
        case SECT_CLOUD:
            sect = "&w@";
            break;
        case SECT_SNOW:
            sect = "&w'";
            break;
        case SECT_ORE:
            sect = "&z#";
            break;
        case ( SECT_MAX + 1 ):
            sect = " ";
            break;
        case ( SECT_MAX + 2 ):                        /* -4 and lower then player level */
            sect = "&z@";
            break;
        case ( SECT_MAX + 3 ):                        /* -3 to +3 levels of player */
            sect = "&G@";
            break;
        case ( SECT_MAX + 4 ):                        /* 4 or 5 levels higher then
                                                        * player by soft range */
            sect = "&Y@";
            break;
        case ( SECT_MAX + 5 ):                        /* Can't enter because of
                                                        * hard_range */
            sect = "&R@";
            break;
        case ( SECT_MAX + 6 ):
            sect = "&Y*";
            break;
    }
    return ( char * ) sect;
}

//Reminder: if ch is "inside", or if outside of winderness vnum range, map doesnt need to display. -Taon
void draw_map( CHAR_DATA *ch, unsigned short int map[50][50] )
{
    int                     x,
                            y;
    const char             *sect;

    /*
     * Default 9x9 Square 
     */
    if ( ch->map_type == 0 ) {
        for ( y = 1; y <= MapLength; ++y ) {
            for ( x = 1; x <= MapWidth; ++x ) {
                sect = map_type( map[x][y] );
                if ( x == centerx && y == centery )
                    sect = "&R*";
                send_to_char_color( sect, ch );
            }
            send_to_char( "\r\n", ch );
        }
    }
    /*
     * This is for the diamond shape 
     */
    else if ( ch->map_type == 1 ) {
        bool                    tmap[50][50];

        for ( y = 0; y < 50; ++y )
            for ( x = 0; x < 50; ++x )
                tmap[x][y] = FALSE;
        tmap[centerx][centery] = TRUE;
        for ( y = 1; y <= MapLength; ++y ) {
            for ( x = 1; x <= MapWidth; ++x ) {
                /*
                 * Top and bottom spot 
                 */
                if ( ( y == 1 || y == MapLength ) && x == centerx )
                    tmap[x][y] = TRUE;
                if ( ( y == 2 || y == ( MapLength - 1 ) ) && x >= ( centerx - 2 )
                     && x <= ( centerx + 2 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 3 || y == ( MapLength - 2 ) ) && x >= ( centerx - 4 )
                     && x <= ( centerx + 4 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 4 || y == ( MapLength - 3 ) ) && x >= ( centerx - 6 )
                     && x <= ( centerx + 6 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 5 || y == ( MapLength - 4 ) ) && x >= ( centerx - 8 )
                     && x <= ( centerx + 8 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 6 || y == ( MapLength - 5 ) ) && x >= ( centerx - 10 )
                     && x <= ( centerx + 10 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 7 || y == ( MapLength - 6 ) ) && x >= ( centerx - 12 )
                     && x <= ( centerx + 12 ) )
                    tmap[x][y] = TRUE;
                if ( y == 8 )
                    tmap[x][y] = TRUE;
            }
        }
        for ( y = 1; y <= MapLength; ++y ) {
            for ( x = 1; x <= MapWidth; ++x ) {
                sect = " ";

                if ( tmap[x][y] == TRUE )
                    sect = map_type( map[x][y] );

                if ( x == centerx && y == centery )
                    sect = "&R*";
                send_to_char_color( sect, ch );
            }
            send_to_char( "\r\n", ch );
        }
    }
    /*
     * This is for the sphere shape 
     */
    else if ( ch->map_type == 2 ) {
        bool                    tmap[50][50];

        for ( y = 0; y < 50; ++y )
            for ( x = 0; x < 50; ++x )
                tmap[x][y] = FALSE;
        tmap[centerx][centery] = TRUE;
        for ( y = 1; y <= MapLength; ++y ) {
            for ( x = 1; x <= MapWidth; ++x ) {
                /*
                 * Top and bottom spot 
                 */
                if ( ( y == 1 || y == MapLength ) && x >= ( centerx - 6 ) && x <= ( centerx + 6 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 2 || y == ( MapLength - 1 ) ) && x >= ( centerx - 8 )
                     && x <= ( centerx + 8 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 3 || y == ( MapLength - 2 ) ) && x >= ( centerx - 9 )
                     && x <= ( centerx + 9 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 4 || y == ( MapLength - 3 ) ) && x >= ( centerx - 11 )
                     && x <= ( centerx + 11 ) )
                    tmap[x][y] = TRUE;
                if ( ( y == 5 || y == ( MapLength - 4 ) ) && x >= ( centerx - 12 )
                     && x <= ( centerx + 12 ) )
                    tmap[x][y] = TRUE;
                if ( ( y >= 6 && y <= ( MapLength - 5 ) ) )
                    tmap[x][y] = TRUE;
            }
        }
        for ( y = 1; y <= MapLength; ++y ) {
            for ( x = 1; x <= MapWidth; ++x ) {
                sect = " ";

                if ( tmap[x][y] == TRUE )
                    sect = map_type( map[x][y] );

                if ( x == centerx && y == centery )
                    sect = "&R*";
                send_to_char_color( sect, ch );
            }
            send_to_char( "\r\n", ch );
        }
    }
}
