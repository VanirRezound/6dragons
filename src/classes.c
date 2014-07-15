//

#include "h/mud.h"
#include "h/files.h"
#include "h/languages.h"
#include "h/key.h"
#ifndef WIN32
#include <unistd.h>
#endif

int                     get_risflag( char *flag );

struct class_type      *class_table[MAX_CLASS];
char                   *title_table[MAX_CLASS][MAX_LEVEL + 1][2];

const char             *const npc_class[MAX_NPC_CLASS] = {
    "priest", "druid", "mage", "necromancer", "warrior", "crusader",
    "thief", "dragon lord", "black", "gold", "silver", "red", "blue", "hellspawn",
    "angel", "vampire", "monk", "bard", "shadowknight", "psionic", "hero",
    "twoheaded", "Balrog", "Sorcerer",
};

void free_all_classes( void )
{
    int                     hash;

    for ( hash = 0; hash < MAX_CLASS; hash++ ) {
        if ( !class_table[hash] )
            continue;
        if ( class_table[hash]->who_name )
            STRFREE( class_table[hash]->who_name );
        if ( class_table[hash]->filename )
            STRFREE( class_table[hash]->filename );
        DISPOSE( class_table[hash] );
    }
    return;
}

/* Create a new class online. -Shaddai */
bool create_new_class( int Class, char *argument )
{
    int                     i;

    if ( Class >= MAX_CLASS || class_table[Class] == NULL || !VLD_STR( argument ) )
        return FALSE;

    if ( class_table[Class]->who_name )
        STRFREE( class_table[Class]->who_name );
    if ( class_table[Class]->filename )
        STRFREE( class_table[Class]->filename );
    if ( argument[0] != '\0' )
        argument[0] = UPPER( argument[0] );
    class_table[Class]->who_name = STRALLOC( argument );
    class_table[Class]->filename = STRALLOC( argument );
    xCLEAR_BITS( class_table[Class]->affected );
    class_table[Class]->attr_prime = 0;
    class_table[Class]->attr_second = 0;
    class_table[Class]->attr_deficient = 0;
    class_table[Class]->race_restriction = 0;
    class_table[Class]->combo_restriction = 0;
    class_table[Class]->resist = 0;
    class_table[Class]->suscept = 0;
    class_table[Class]->skill_adept = 0;
    class_table[Class]->thac0_00 = 0;
    class_table[Class]->thac0_32 = 0;
    class_table[Class]->hp_min = 0;
    class_table[Class]->hp_max = 0;
    class_table[Class]->mana_min = 0;
    class_table[Class]->mana_max = 0;
    class_table[Class]->starting = FALSE;              /* Default to not a starting class 
                                                        */
    class_table[Class]->exp_base = 0;
    class_table[Class]->craft_base = 0;
    for ( i = 0; i < MAX_LEVEL; i++ ) {
        title_table[Class][i][0] = STRALLOC( "Not set." );
        title_table[Class][i][1] = STRALLOC( "Not set." );
    }
    return TRUE;
}

void write_class_list( void )
{
    char                    classlist[256];
    FILE                   *fpList;
    int                     i;

    snprintf( classlist, 256, "%s%s", CLASSDIR, CLASS_LIST );

    if ( ( fpList = FileOpen( classlist, "w" ) ) == NULL ) {
        bug( "Can't open %s %s Class list for writing.", __FUNCTION__, CLASS_LIST );
        perror( CLASS_LIST );
        return;
    }
    for ( i = 0; i < MAX_PC_CLASS; i++ )
//    fprintf(fpList, "%s%s.class\n", CLASSDIR, class_table[i]->filename);  Volk - bugging the file LOLOL
        fprintf( fpList, "%s.class\n", class_table[i]->filename );
    fprintf( fpList, "$\n" );
    FileClose( fpList );
}

/* Edit class information -Thoric */
void do_setclass( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    struct class_type      *Class;
    int                     cl,
                            value,
                            i;

    set_char_color( AT_PLAIN, ch );
    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Syntax: setclass <class> <field> <value>\r\n", ch );
        send_to_char( "Syntax: setclass <class> create\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( "  name filename prime thac0 thac32 nocombo\r\n", ch );
        send_to_char( "  hpmin hpmax manamin manamax expbase mtitle ftitle\r\n", ch );
        send_to_char( "  second, deficient affected resist suscept\r\n", ch );
        send_to_char( "  race starting reclass1 reclass2 reclass3\r\n", ch );
        return;
    }
    if ( is_number( arg1 ) && ( cl = atoi( arg1 ) ) >= 0 && cl < MAX_CLASS )
        Class = class_table[cl];
    else {
        Class = NULL;
        for ( cl = 0; cl < MAX_CLASS && class_table[cl]; cl++ ) {
            if ( !class_table[cl]->who_name )
                continue;
            if ( !str_cmp( class_table[cl]->who_name, arg1 ) ) {
                Class = class_table[cl];
                break;
            }
        }
    }
    if ( !str_cmp( arg2, "create" ) && Class ) {
        send_to_char( "That class already exists!\r\n", ch );
        return;
    }
    if ( !Class && str_cmp( arg2, "create" ) ) {
        send_to_char( "No such class.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "save" ) ) {
        DESCRIPTOR_DATA        *d;

        write_class_file( cl );
        send_to_char( "&CUpdating class for all players now.\r\n", ch );
        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->character )
                update_aris( d->character );
        }
        return;
    }
    if ( !str_cmp( arg2, "create" ) ) {
        if ( MAX_PC_CLASS >= MAX_CLASS ) {
            send_to_char( "You need to up MAX_CLASS in mud and make clean.\r\n", ch );
            return;
        }
        if ( ( create_new_class( MAX_PC_CLASS, arg1 ) ) == FALSE ) {
            send_to_char( "Couldn't create a new class.\r\n", ch );
            return;
        }
        write_class_file( MAX_PC_CLASS );
        MAX_PC_CLASS++;
        write_class_list(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !argument ) {
        send_to_char( "You must specify an argument.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Can't set a class name to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( Class->who_name ) )
            STRFREE( Class->who_name );
        Class->who_name = STRALLOC( ( argument ) );
        send_to_char( "Class name is set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Can't set a filename to nothing.\r\n", ch );
            return;
        }
        if ( VLD_STR( Class->filename ) )
            STRFREE( Class->filename );
        Class->filename = STRALLOC( ( argument ) );
        send_to_char( "Filename is set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "starting" ) ) {
        Class->starting = !Class->starting;

        ch_printf( ch, "That class is now a %s class.\r\n",
                   Class->starting ? "starting" : "non-starting" );
        return;
    }

    // Start of the nocombo aurgument! -Taon

    if ( !str_cmp( arg2, "nocombo" ) ) {
        for ( i = 0; i < MAX_CLASS; i++ ) {
            if ( class_table[i] && VLD_STR( class_table[i]->who_name ) ) {
                if ( !str_cmp( argument, class_table[i]->who_name ) ) {
                    TOGGLE_BIT( Class->combo_restriction, 1 << i );
                    send_to_char( "Done!\r\n", ch );
                    send_to_char
                        ( "Note this still isnt complete, as far as the nanny function is concerned!\r\n",
                          ch );
                    return;
                }
            }
        }
        return;
    }
    if ( !str_cmp( arg2, "race" ) ) {
        for ( i = 0; i < MAX_RACE; i++ ) {
            if ( !str_cmp( argument, race_table[i]->race_name ) ) {
                TOGGLE_BIT( Class->race_restriction, 1 << i );  /* k, that's boggling */
                send_to_char( "Done.\r\n", ch );
                return;
            }
        }
        send_to_char( "No such race.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "reclass1" ) ) {
        Class->reclass1 = 0;
        for ( i = 0; i < MAX_CLASS; i++ ) {
            if ( class_table[i] && VLD_STR( class_table[i]->who_name ) ) {
                if ( !str_cmp( argument, class_table[i]->who_name ) ) {
                    Class->reclass1 = i;
                    send_to_char( "Done.\r\n", ch );
                    return;
                }
            }
        }
        send_to_char( "No such class. Reclass1 cleared.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "reclass2" ) ) {
        Class->reclass2 = 0;
        for ( i = 0; i < MAX_CLASS; i++ ) {
            if ( class_table[i] && VLD_STR( class_table[i]->who_name ) ) {
                if ( !str_cmp( argument, class_table[i]->who_name ) ) {
                    Class->reclass2 = i;
                    send_to_char( "Done.\r\n", ch );
                    return;
                }
            }
        }
        send_to_char( "No such class. Reclass2 cleared.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "reclass3" ) ) {
        Class->reclass3 = 0;
        for ( i = 0; i < MAX_CLASS; i++ ) {
            if ( class_table[i] && VLD_STR( class_table[i]->who_name ) ) {
                if ( !str_cmp( argument, class_table[i]->who_name ) ) {
                    Class->reclass3 = i;
                    send_to_char( "Done.\r\n", ch );
                    return;
                }
            }
        }
        send_to_char( "No such class. Reclass3 cleared.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "second" ) ) {
        int                     x = get_atype( argument );

        if ( x < APPLY_STR || ( x > APPLY_CON && x != APPLY_LCK && x != APPLY_CHA ) )
            send_to_char( "Invalid second attribute!\r\n", ch );
        else {
            Class->attr_second = x;
            send_to_char( "Done.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "affected" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setclass <class> affected <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg2 );
            value = get_aflag( arg2 );
            if ( value < 0 || value > MAX_BITS )
                ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
            else
                xTOGGLE_BIT( Class->affected, value );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "resist" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setclass <class> resist <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg2 );
            value = get_risflag( arg2 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
            else
                TOGGLE_BIT( Class->resist, 1 << value );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "suscept" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "Usage: setclass <class> suscept <flag> [flag]...\r\n", ch );
            return;
        }
        while ( VLD_STR( argument ) ) {
            argument = one_argument( argument, arg2 );
            value = get_risflag( arg2 );
            if ( value < 0 || value > 31 )
                ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
            else
                TOGGLE_BIT( Class->suscept, 1 << value );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "deficient" ) ) {
        int                     x = get_atype( argument );

        if ( x < APPLY_STR || ( x > APPLY_CON && x != APPLY_LCK && x != APPLY_CHA ) )
            send_to_char( "Invalid deficient attribute!\r\n", ch );
        else {
            Class->attr_deficient = x;
            send_to_char( "Done.\r\n", ch );
        }
        return;
    }
    if ( !str_cmp( arg2, "prime" ) ) {
        int                     x = get_atype( argument );

        if ( x < APPLY_STR || ( x > APPLY_CON && x != APPLY_LCK && x != APPLY_CHA ) )
            send_to_char( "Invalid prime attribute!\r\n", ch );
        else {
            Class->attr_prime = x;
            send_to_char( "Done.\r\n", ch );
        }
        return;
    }
    if ( !str_cmp( arg2, "thac0" ) ) {
        Class->thac0_00 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "thac32" ) ) {
        Class->thac0_32 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "hpmin" ) ) {
        Class->hp_min = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "hpmax" ) ) {
        Class->hp_max = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "manamin" ) ) {
        Class->mana_min = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "manamax" ) ) {
        Class->mana_max = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "expbase" ) ) {
        Class->exp_base = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "craftbase" ) ) {
        Class->craft_base = atoi( argument );
        send_to_char( "Craft base is now set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "mtitle" ) ) {
        char                    arg3[MIL];
        int                     x;

        argument = one_argument( argument, arg3 );
        if ( arg3[0] == '\0' || argument[0] == '\0' ) {
            send_to_char( "Syntax: setclass <class> mtitle <level> <title>\r\n", ch );
            return;
        }
        if ( !VLD_STR( argument ) ) {
            send_to_char( "What would you like to set the mtitle_table to?\r\n", ch );
            return;
        }
        if ( ( x = atoi( arg3 ) ) < 0 || x > MAX_LEVEL ) {
            send_to_char( "Invalid level.\r\n", ch );
            return;
        }
        STRFREE( title_table[cl][x][0] );
        title_table[cl][x][0] = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "ftitle" ) ) {
        char                    arg3[MIL];
        int                     x;

        argument = one_argument( argument, arg3 );
        if ( arg3[0] == '\0' || argument[0] == '\0' ) {
            send_to_char( "Syntax: setclass <class> ftitle <level> <title>\r\n", ch );
            return;
        }
        if ( !VLD_STR( argument ) ) {
            send_to_char( "What would you like to set the ftitle_table to?\r\n", ch );
            return;
        }
        if ( ( x = atoi( arg3 ) ) < 0 || x > MAX_LEVEL ) {
            send_to_char( "Invalid level.\r\n", ch );
            return;
        }
        STRFREE( title_table[cl][x][1] );
        title_table[cl][x][1] = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    do_setclass( ch, ( char * ) "" );
    return;
}

/* Display class information -Thoric */
void do_showclass( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL];
    struct class_type      *Class;
    int                     cl,
                            low,
                            hi,
                            ct,
                            i;

    set_pager_color( AT_PLAIN, ch );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) ) {
        do_classes( ch, ( char * ) "" );
        send_to_char( "Syntax: showclass <class> [level range]\r\n", ch );
        return;
    }
    if ( is_number( arg1 ) && ( cl = atoi( arg1 ) ) >= 0 && cl < MAX_CLASS )
        Class = class_table[cl];
    else {
        Class = NULL;
        for ( cl = 0; cl < MAX_CLASS && class_table[cl]; cl++ )
            if ( !str_cmp( class_table[cl]->who_name, arg1 ) ) {
                Class = class_table[cl];
                break;
            }
    }
    if ( !Class ) {
        send_to_char( "No such class.\r\n", ch );
        return;
    }
    pager_printf( ch, "&wCLASS: &W%s\r\n&w",
                  VLD_STR( Class->who_name ) ? Class->who_name : "(Not set)" );
    pager_printf( ch, "&wFilename: &W%s\r\n&w",
                  VLD_STR( Class->filename ) ? Class->filename : "(Not set)" );
    pager_printf( ch, "&wPrime Attribute:     &W%-14s\r\n", a_types[Class->attr_prime] );
    pager_printf( ch, "&wSecond Attribute:    &W%-14s\r\n", a_types[Class->attr_second] );
    pager_printf( ch, "&wDeficient Attribute: &W%-14s\r\n", a_types[Class->attr_deficient] );
    ct = 0;
    send_to_pager( "&wDisallowed Races:&W\r\n", ch );
    for ( i = 0; i < MAX_RACE; i++ ) {
        if ( IS_SET( Class->race_restriction, 1 << i ) ) {
            ct++;
            pager_printf( ch, "%10s ", race_table[i]->race_name );
            if ( ct % 6 == 0 )
                send_to_pager( "\r\n", ch );
        }
    }
    if ( ( ct % 6 != 0 ) || ( ct == 0 ) )
        send_to_pager( "\r\n", ch );
    ct = 0;
    send_to_pager( "&wAllowed Races:&W\r\n", ch );
    for ( i = 0; i < MAX_RACE; i++ ) {
        if ( !IS_SET( Class->race_restriction, 1 << i ) ) {
            ct++;
            pager_printf( ch, "%10s ", race_table[i]->race_name );
            if ( ct % 6 == 0 )
                send_to_pager( "\r\n", ch );
        }
    }

    // Added in combo restrictions by: Taon...
/*
  if((ct % 6 != 0) || (ct == 0))
    send_to_pager("\r\n", ch);
  ct = 0;

  send_to_pager("&wAllowed combos:&W\r\n", ch);
  for(i = 0; i < MAX_CLASS; i++)
  {
    ct++;

    if(!IS_SET(Class->combo_restriction, 1 << i))
    {
      pager_printf(ch, "%10s ", class_table[i]->who_name);
      send_to_pager("\r\n", ch);
    }
  }
  if((ct % 6 != 0) || (ct == 0))
    send_to_pager("\r\n", ch);
  ct = 0;

  send_to_pager("&wNot Allowed combos:&W\r\n", ch);
  for(i = 0; i < MAX_CLASS; i++)
  {
    ct++;
    if(IS_SET(Class->combo_restriction, 1 << i))
    {
      pager_printf(ch, "%10s ", class_table[i]->who_name);
      send_to_pager("\r\n", ch);
    }
  }
*/
    send_to_char( "\r\n", ch );
    pager_printf( ch,
                  "&wMax Skill Adept: &W%-3d             &wThac0 : &W%-5d     &wThac32: &W%d\r\n",
                  Class->skill_adept, Class->thac0_00, Class->thac0_32 );
    pager_printf( ch,
                  "&wHp Min/Hp Max  : &W%-2d/%-2d     &wMana Min/Max: &W%-2d/%-2d &wExpBase: &W%d\r\n",
                  Class->hp_min, Class->hp_max, Class->mana_min, Class->mana_max, Class->exp_base );
    pager_printf( ch, "&W%s &wClass\r\n", Class->starting ? "Starting" : "Non-Starting" );
    pager_printf( ch, "&wAffected by:  &W%s\r\n",
                  !xIS_EMPTY( Class->affected ) ? ext_flag_string( &Class->affected,
                                                                   a_flags ) : "Nothing" );
    pager_printf( ch, "&wResistant to: &W%s\r\n", flag_string( Class->resist, ris_flags ) );
    pager_printf( ch, "&wSusceptible to: &W%s\r\n", flag_string( Class->suscept, ris_flags ) );
    pager_printf( ch, "&wCrafting Base: &W%d\r\n", Class->craft_base );

    /*
     * These are no longer in use. -Taon if(Class->reclass1 > 0) pager_printf(ch, "%s ",
     * class_table[class->reclass1]->who_name); if(Class->reclass2 > 0) pager_printf(ch,
     * "%s ", class_table[class->reclass2]->who_name); if(Class->reclass3 > 0)
     * pager_printf(ch, "%s ", class_table[class->reclass3]->who_name);
     * send_to_pager("\r\n", ch); 
     */

/*
  if(VLD_STR(arg2))
  {
    int x, y, cnt;

    low = UMAX(0, atoi(arg2));
    hi = URANGE(low, atoi(argument), MAX_LEVEL);
    for(x = low; x <= hi; x++)
    {
      set_pager_color(AT_LBLUE, ch);
      pager_printf(ch, "Male: %-30s Female: %s\r\n", title_table[cl][x][0], title_table[cl][x][1]);
      cnt = 0;
      set_pager_color(AT_BLUE, ch);
      for(y = gsn_first_spell; y < gsn_top_sn; y++)
        if(skill_table[y]->skill_level[cl] == x)
        {
          pager_printf(ch, "  %-7s %-19s%3d     ", skill_tname[skill_table[y]->type], skill_table[y]->name, skill_table[y]->skill_adept[cl]);
          if(++cnt % 2 == 0)
            send_to_pager("\r\n", ch);
          if(cnt % 2 != 0)
            send_to_pager("\r\n", ch);
          send_to_pager("\r\n", ch);
        }
    }
  }
*/

    if ( arg2 && arg2[0] != '\0' ) {
        int                     x,
                                y,
                                cnt;

        low = UMAX( 0, atoi( arg2 ) );
        hi = URANGE( low, atoi( argument ), MAX_LEVEL );
        for ( x = low; x <= hi; x++ ) {
            pager_printf( ch, "&wMale: &W%-30s &wFemale: &W%s\r\n", title_table[cl][x][0],
                          title_table[cl][x][1] );
            cnt = 0;
            for ( y = gsn_first_spell; y < gsn_top_sn; y++ )
                if ( skill_table[y]->skill_level[cl] == x ) {
                    pager_printf( ch, "  &[skill]%-7s %-19s%3d     ",
                                  skill_tname[skill_table[y]->type], skill_table[y]->name,
                                  skill_table[y]->skill_adept[cl] );
                    if ( ++cnt % 2 == 0 )
                        send_to_pager( "\r\n", ch );
                }
            if ( cnt % 2 != 0 )
                send_to_pager( "\r\n", ch );
            send_to_pager( "\r\n", ch );
        }
    }

    return;
}

void do_classes( CHAR_DATA *ch, char *argument )
{
    int                     iClass = 0;
    int                     counter = 0;

    if ( !IS_IMMORTAL( ch ) )
        counter = 1;
    send_to_pager( "\r\n", ch );
    for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
        if ( !class_table[iClass] || !VLD_STR( class_table[iClass]->who_name ) ) {
            if ( IS_IMMORTAL( ch ) )
                counter++;
            continue;
        }
        pager_printf( ch, "&c[&C%2d&c]&W %-15s", counter, class_table[iClass]->who_name );
        if ( IS_IMMORTAL( ch ) )
            pager_printf( ch,
                          " &cStarting:&W%3s &cExpbase:&W %-5d &cPrime: &W%-14s &cMana: &W%2d-%-2d",
                          class_table[iClass]->starting ? "Yes" : "No",
                          class_table[iClass]->exp_base, a_types[class_table[iClass]->attr_prime],
                          class_table[iClass]->mana_min, class_table[iClass]->mana_max );
        pager_printf( ch, "&D\r\n" );
        counter++;
    }
    return;
}

bool load_class_file( const char *fname )
{
    char                    buf[MSL];
    const char             *word;
    bool                    fMatch;
    struct class_type      *Class;
    int                     cl = -1,
        tlev = 0;
    FILE                   *fp;

    snprintf( buf, MSL, "%s%s", CLASSDIR, fname );
    if ( ( fp = FileOpen( buf, "r" ) ) == NULL ) {
        perror( buf );
        return FALSE;
    }
    CREATE( Class, struct class_type, 1 );
    /*
     * Setup defaults for additions to class structure
     */
    Class->starting = FALSE;
    Class->attr_second = 0;
    Class->attr_deficient = 0;
    xCLEAR_BITS( Class->affected );
    Class->resist = 0;
    Class->suscept = 0;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "Affected", Class->affected, fread_bitvector( fp ) );
                KEY( "AttrPrime", Class->attr_prime, fread_number( fp ) );
                KEY( "AttrSecond", Class->attr_second, fread_number( fp ) );
                KEY( "AttrDeficient", Class->attr_deficient, fread_number( fp ) );
                break;
            case 'C':
                KEY( "Class", cl, fread_number( fp ) );
                KEY( "Craftbase", Class->craft_base, fread_number( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                    if ( !str_cmp( word, "End" ) ) {
                        FileClose( fp );
                        if ( cl < 0 || cl >= MAX_CLASS ) {
                            bug( "Load_class_file: Class (%s) bad/not found (%d)",
                                 Class->who_name ? Class->who_name : "name not found", cl );
                            if ( VLD_STR( Class->who_name ) )
                                STRFREE( Class->who_name );
                            if ( VLD_STR( Class->filename ) )
                                STRFREE( Class->filename );
                            DISPOSE( Class );
                            return FALSE;
                        }
                        if ( !Class->filename )
                            Class->filename = STRALLOC( Class->who_name );
                        class_table[cl] = Class;
                        return TRUE;
                    }
                KEY( "ExpBase", Class->exp_base, fread_number( fp ) );
                break;
            case 'F':
                KEY( "Filename", Class->filename, fread_string( fp ) );
            case 'H':
                KEY( "HpMax", Class->hp_max, fread_number( fp ) );
                KEY( "HpMin", Class->hp_min, fread_number( fp ) );
                break;
            case 'M':
                KEY( "ManaMax", Class->mana_max, fread_number( fp ) );
                KEY( "ManaMin", Class->mana_min, fread_number( fp ) );
                break;
            case 'N':
                KEY( "Nocombo", Class->combo_restriction, fread_number( fp ) );
                KEY( "Name", Class->who_name, fread_string( fp ) );
                break;
            case 'R':
                KEY( "Races", Class->race_restriction, fread_number( fp ) );
                if ( !str_cmp( word, "Reclass" ) ) {
                    Class->reclass1 = fread_number( fp );
                    Class->reclass2 = fread_number( fp );
                    Class->reclass3 = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Resist", Class->resist, fread_number( fp ) );
                break;
            case 'S':
                KEY( "Starting", Class->starting, fread_number( fp ) );
                if ( !str_cmp( word, "Skill" ) ) {
                    int                     sn,
                                            lev,
                                            adp;

                    word = fread_word( fp );
                    lev = fread_number( fp );
                    adp = fread_number( fp );
                    sn = skill_lookup( word );
                    if ( cl < 0 || cl >= MAX_CLASS )
                        bug( "load_class_file: Skill %s -- class bad/not found (%d)", word, cl );
                    else if ( !IS_VALID_SN( sn ) )
                        bug( "load_class_file: Skill %s unknown", word );
                    else {
                        skill_table[sn]->skill_level[cl] = lev;
                        skill_table[sn]->skill_adept[cl] = adp;
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "Skilladept", Class->skill_adept, fread_number( fp ) );
                KEY( "Suscept", Class->suscept, fread_number( fp ) );
                break;
            case 'T':
                if ( !str_cmp( word, "Title" ) ) {
                    if ( cl < 0 || cl >= MAX_CLASS ) {
                        char                   *tmp;

                        bug( "load_class_file: Title -- class bad/not found (%d)", cl );
                        tmp = fread_string( fp );
                        DISPOSE( tmp );
                        tmp = fread_string( fp );
                        DISPOSE( tmp );
                    }
                    else if ( tlev < MAX_LEVEL + 1 ) {
                        title_table[cl][tlev][0] = fread_string( fp );
                        title_table[cl][tlev][1] = fread_string( fp );
                        ++tlev;
                    }
                    else
                        bug( "%s", "load_class_file: Too many titles" );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Thac0", Class->thac0_00, fread_number( fp ) );
                KEY( "Thac32", Class->thac0_32, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "load_class_file: no match: %s", word );
            fread_to_eol( fp );
        }
    }
    return FALSE;
}

/* Load in all the class files. */
void load_classes(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    classlist[256];
    int                     i;

    MAX_PC_CLASS = 0;

    /*
     * Pre-init the class_table with blank classes
     */
    for ( i = 0; i < MAX_CLASS; ++i )
        class_table[i] = NULL;

    snprintf( classlist, 256, "%s%s", CLASSDIR, CLASS_LIST );
    if ( ( fpList = FileOpen( classlist, "r" ) ) == NULL ) {
        bug( "%s: Can't open %s for reading", __FUNCTION__, CLASS_LIST );
        perror( classlist );
        exit( 1 );
    }

    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        if ( filename[0] == '$' )
            break;

        if ( !load_class_file( filename ) ) {
            bug( "%s Cannot load class file: %s", __FUNCTION__, filename );
        }
        else
            ++MAX_PC_CLASS;
    }
    FileClose( fpList );
    for ( i = 0; i < MAX_CLASS; ++i ) {
        if ( class_table[i] == NULL ) {
            CREATE( class_table[i], struct class_type, 1 );

            create_new_class( i, ( char * ) "" );
        }
    }
    return;
}

void write_class_file( int cl )
{
    FILE                   *fpout;
    char                    filename[MIL];
    struct class_type      *Class = class_table[cl];
    int                     x,
                            y;

    snprintf( filename, MIL, "%s%s.class", CLASSDIR, Class->filename );
    if ( ( fpout = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "Cannot open: %s for writing", filename );
        return;
    }
    if ( VLD_STR( Class->who_name ) )
        fprintf( fpout, "Name           %s~\n", Class->who_name );
    if ( VLD_STR( Class->filename ) )
        fprintf( fpout, "Filename       %s~\n", Class->filename );
    fprintf( fpout, "Class          %d\n", cl );
    fprintf( fpout, "Starting       %d\n", Class->starting );
    fprintf( fpout, "AttrPrime      %d\n", Class->attr_prime );
    fprintf( fpout, "AttrSecond     %d\n", Class->attr_second );
    fprintf( fpout, "AttrDeficient  %d\n", Class->attr_deficient );
    fprintf( fpout, "Races          %d\n", Class->race_restriction );
    fprintf( fpout, "Nocombo        %d\n", Class->combo_restriction );
    fprintf( fpout, "Reclass        %d %d %d\n", Class->reclass1, Class->reclass2,
             Class->reclass3 );
    fprintf( fpout, "Skilladept     %d\n", Class->skill_adept );
    fprintf( fpout, "Thac0          %d\n", Class->thac0_00 );
    fprintf( fpout, "Thac32         %d\n", Class->thac0_32 );
    fprintf( fpout, "Hpmin          %d\n", Class->hp_min );
    fprintf( fpout, "Hpmax          %d\n", Class->hp_max );
    fprintf( fpout, "ManaMin        %d\n", Class->mana_min );
    fprintf( fpout, "ManaMax        %d\n", Class->mana_max );
    fprintf( fpout, "Expbase        %d\n", Class->exp_base );
    fprintf( fpout, "Craftbase        %d\n", Class->craft_base );
    if ( !xIS_EMPTY( Class->affected ) )
        fprintf( fpout, "Affected       %s\n", print_bitvector( &Class->affected ) );
    fprintf( fpout, "Resist         %d\n", Class->resist );
    fprintf( fpout, "Suscept        %d\n", Class->suscept );
    for ( x = 0; x < top_sn; x++ ) {
        if ( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
            break;
        if ( ( y = skill_table[x]->skill_level[cl] ) < LEVEL_IMMORTAL )
            fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y,
                     skill_table[x]->skill_adept[cl] );
    }
    for ( x = 0; x <= MAX_LEVEL; x++ )
        fprintf( fpout, "Title\n%s~\n%s~\n", title_table[cl][x][0], title_table[cl][x][1] );
    fprintf( fpout, "End\n" );
    FileClose( fpout );
}

void save_classes(  )
{
    int                     x;

    for ( x = 0; x < MAX_PC_CLASS; x++ )
        write_class_file( x );
}
