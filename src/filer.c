#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "h/mud.h"

char                   *str_replace( const char *seed, const char *apple, char *tree );

FILE                   *__FileOpen( const char *filename, const char *mode, const char *file,
                                    const char *function, int line )
{
    FILE                   *fp = NULL;
    char                    fbuf[256];

    if ( !filename || filename[0] == '\0' || !mode || mode[0] == '\0' ) {
        log_string( "FileOpen called improperly." );
        return NULL;
    }

    // If writing...first create a temp file, otherwise just open the file -->KeB
    // 10/29/08
    if ( strstr( mode, "w" ) )
        snprintf( fbuf, 256, "%s.temporary", filename );
    else
        snprintf( fbuf, 256, "%s", filename );

    if ( ( fp = fopen( fbuf, mode ) ) == NULL ) {
        bug( "%s: can't open %s for %s. Called from %s:%d", __FUNCTION__, fbuf, mode, file, line );
        perror( fbuf );
        return NULL;
    }
    else {
        // *If you want to be really picky, define this* //
#ifdef DEBUG_FILEDATA
        FILE_DATA              *file_data;

        for ( file_data = file_list; file_data; file_data->next ) {
            if ( file_data->fp == fp ) {
                log_string( "FileOpen: Double opening of a file!" );
            }
        }
#endif

        FILE_DATA              *filedata;

        CREATE( filedata, FILE_DATA, 1 );
        filedata->filename = STRALLOC( fbuf );
        filedata->mode = STRALLOC( mode );
        filedata->file = STRALLOC( file );
        filedata->function = STRALLOC( function );
        filedata->line = line;
        filedata->fp = fp;
        LINK( filedata, first_filedata, last_filedata, next, prev );
        FilesOpen++;
    }

    return fp;
}

void free_filedata( FILE_DATA * filedata )
{
    UNLINK( filedata, first_filedata, last_filedata, next, prev );
    STRFREE( filedata->filename );
    STRFREE( filedata->file );
    STRFREE( filedata->function );
    STRFREE( filedata->mode );
    filedata->fp = NULL;
    DISPOSE( filedata );
}

void free_all_filedata( void )
{
    FILE_DATA              *filedata,
                           *filedata_next;

    for ( filedata = first_filedata; filedata; filedata = filedata_next ) {
        filedata_next = filedata->next;
        free_filedata( filedata );
    }
    return;
}

// *Close the file-data* //
void FileClose( FILE * fp )
{
    FILE_DATA              *filedata,
                           *filedata_next;
    char                    new_fname[MIL];
    char                    old_fname[MIL];

    new_fname[0] = '\0';

    if ( !fp )
        return;

    for ( filedata = first_filedata; filedata; filedata = filedata_next ) {
        filedata_next = filedata->next;

        if ( filedata->fp == fp ) {
            if ( !str_suffix( ".temporary", filedata->filename ) ) {
                snprintf( old_fname, MIL, "%s", filedata->filename );
                snprintf( new_fname, MIL, "%s", filedata->filename );
                str_replace( ".temporary", "", new_fname );
            }
            free_filedata( filedata );
            break;
        }
    }

    fclose( fp );
    fp = NULL;
    FilesOpen--;

    if ( FilesOpen < 0 ) {
        FilesOpen = 0;
        log_string( "FileClose passed a null fp somewhere and schewed the list." );
    }

    if ( new_fname[0] != '\0' ) {
        if ( rename( old_fname, new_fname ) ) {
            log_printf( "FileClose: Problem with renaming %s to %s", old_fname, new_fname );
            return;
        }
    }
}

// *ALL files should be closed.  I mean that, there is 100% no need to have an open file, unless your a sloppy coder* //
// *Remember that, what this does is link all the open file-data, and display it to you, this will allow you to find out * //
// *Where, and why your mud is crapping out.   I plugged this into a base, i forget which one, and after it compiled * //
// *I booted the mud up, and when i typed fileio, the damned thing had almost 30 open files because they didn't * //
// *Close the files properly, so i recommend this to people, because, if you have open files.  This will be a perfect* //
// * Way to find them, see exactly where they were opened, and why (mode) thus allowing you to find why they.* //
// *didn't close* //

void do_fileio( CHAR_DATA *ch, char *argument )
{
    FILE_DATA              *filedata;
    char                    buf[MSL];
    int                     count = 0;

    pager_printf( ch,
                  "        &YFilename             &wMode      &WOpened                     &CFunction         &OLine\r\n" );
    send_to_pager
        ( "&c------------------------------------------------------------------------------------------\r\n",
          ch );
    if ( !first_filedata )
        send_to_pager( "\r\n&RCongrats, &Yyou have no &WOpen &Yfiles!\r\n", ch );

    for ( filedata = first_filedata; filedata; filedata = filedata->next ) {
        sprintf( buf, "&Y%-25.25s     &w%-1.1s       &W%-20.20s     &C%-15.15s     &O%-4.4d\r\n",
                 filedata->filename, filedata->mode, filedata->file, filedata->function,
                 filedata->line );
        count++;
        send_to_pager( buf, ch );
    }

    // *Add to the evil* //
    send_to_pager( "\r\n", ch );

    // *Make sure the count is right.* //
    if ( FilesOpen != count ) {
        send_to_pager( "&RThats Odd, the FilesOpen and count don't match!!!!", ch );
    }
    return;
}

char                   *str_replace( const char *seed, const char *apple, char *tree )
{
    static char             buffer[MSL * 2];           // working buffer
    char                   *nail;                      // pointer to location in tree
    char                   *bpt;                       // pointer to location in buffer

    buffer[0] = '\0';                                  // initialize static buffer
    bpt = &buffer[0];                                  // point bpt to first char of
    // buffer
    nail = &tree[0];                                   // point nail to first char of
    // tree
    while ( *nail != '\0' )                            // while nail doesn't point to end
    {
        if ( strncasecmp( nail, seed, strlen( seed ) ) )    // if seed isn't found
        {
            strncat( bpt, nail, 1 );                   // append first char of nail to
            // bpt
            bpt++;
            nail++;                                    // increment them to the next char
        }
        else                                           // if seed was found
        {
            strcat( buffer, apple );                   // tag apple onto buffer
            bpt += strlen( apple );                    // increment bpt by length of
            // apple
            nail += strlen( seed );                    // increment nail by length of
            // seed
        }
    }
    *bpt++ = '\0';                                     // add null character to end of
    // buffer
    strcpy( tree, buffer );                            // copy buffer into original
    // string
    return buffer;                                     // and return buffer
}
