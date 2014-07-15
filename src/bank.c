/***************************************************************************  
 *                          SMAUG Banking Support Code                     *
 ***************************************************************************
 *                                                                         *
 * This code may be used freely, as long as credit is given in the help    *
 * file. Thanks.                                                           *
 *                                                                         *
 *                                        -= Minas Ravenblood =-           *
 *                                 Implementor of The Apocalypse Theatre   *
 *                                      (email: krisco7@hotmail.com)       *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 * Taltos's Bank code includes:                                            *
 * Bank account each with a password and a possibility of more than one    *
 * per character and also will allow sharing of money between players      *
 * if wanted.                                                              *
 * Also 2 commands for immortals to view  and edit accounts' details       *
 *                                                                         *
 *                                                 By: Taltos              *
 *                                               AIM: jmoder51             *
 *                                    --|-- email: brainbuster51@yahoo.com *
 *                            O        |||--------------------\            *
 *                          -OO=========||___________________ /            *
 *                            O        |||--------------------\            *
 *                                    --|--                                *
 ***************************************************************************/

#include "h/mud.h"
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "h/files.h"
#include "h/currency.h"

void                    free_bank_to_chars( BANK_DATA * bank );

CHAR_DATA              *find_banker( CHAR_DATA *ch )
{
    CHAR_DATA              *banker = NULL;

    for ( banker = ch->in_room->first_person; banker; banker = banker->next_in_room )
        if ( IS_NPC( banker ) && xIS_SET( banker->act, ACT_BANKER ) )
            break;

    return banker;
}

void unlink_bank( BANK_DATA * bank )
{
    BANK_DATA              *tmp,
                           *tmp_next;
    int                     hash;

    if ( !bank ) {
        bug( "Unlink_bank: NULL bank" );
        return;
    }

    if ( bank->name[0] < 'a' || bank->name[0] > 'z' )
        hash = 0;
    else
        hash = ( bank->name[0] - 'a' ) + 1;

    if ( bank == ( tmp = bank_index[hash] ) ) {
        bank_index[hash] = tmp->next;
        return;
    }
    for ( ; tmp; tmp = tmp_next ) {
        tmp_next = tmp->next;
        if ( bank == tmp_next ) {
            tmp->next = tmp_next->next;
            return;
        }
    }
}

void free_bank( BANK_DATA * bank )
{
    if ( bank->name )
        STRFREE( bank->name );
    if ( bank->password )
        STRFREE( bank->password );
    DISPOSE( bank );
}

void free_banks( void )
{
    BANK_DATA              *bank,
                           *bank_next;
    int                     hash;

    for ( hash = 0; hash < 126; hash++ ) {
        for ( bank = bank_index[hash]; bank; bank = bank_next ) {
            bank_next = bank->next;
            unlink_bank( bank );
            free_bank( bank );
        }
    }
}

void add_bank( BANK_DATA * bank )
{
    int                     hash,
                            x;
    BANK_DATA              *tmp,
                           *prev;

    if ( !bank ) {
        bug( "Add_bank: NULL bank" );
        return;
    }

    if ( !bank->name ) {
        bug( "Add_bank: NULL bank->name" );
        return;
    }

    if ( !bank->password ) {
        bug( "Add_bank: NULL bank->password" );
        return;
    }

    /*
     * make sure the name is all lowercase 
     */
    for ( x = 0; bank->name[x] != '\0'; x++ )
        bank->name[x] = LOWER( bank->name[x] );
    if ( bank->name[0] < 'a' || bank->name[0] > 'z' )
        hash = 0;
    else
        hash = ( bank->name[0] - 'a' ) + 1;

    if ( ( prev = tmp = bank_index[hash] ) == NULL ) {
        bank->next = bank_index[hash];
        bank_index[hash] = bank;
        return;
    }

    for ( ; tmp; tmp = tmp->next ) {
        if ( ( x = strcmp( bank->name, tmp->name ) ) == 0 ) {
            bug( "Add_bank: trying to add duplicate name to bucket %d", hash );
            free_bank( bank );
            return;
        }
        else if ( x < 0 ) {
            if ( tmp == bank_index[hash] ) {
                bank->next = bank_index[hash];
                bank_index[hash] = bank;
                return;
            }
            prev->next = bank;
            bank->next = tmp;
            return;
        }
        prev = tmp;
    }

    /*
     * add to end 
     */
    prev->next = bank;
    bank->next = NULL;
    return;
}

void do_account( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    CHAR_DATA              *banker;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    arg3[MAX_INPUT_LENGTH];
    char                    buf[MSL],
                           *pwdnew,
                           *p;
    int                     currtime = time( 0 );

    /*
     * arg1 == account name
     * arg2 == password (if none, close it)
     */

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) || IS_IMMORTAL( ch ) ) {
        snprintf( buf, MSL, "say Sorry, %s, we don't do business with mobs, or 6D STAFF.",
                  ch->name );
        interpret( banker, buf );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Syntax: account [account name] [account password]\n", ch );
        send_to_char( "Syntax: account [account name] [account password] [create/delete]\n", ch );
        send_to_char( "Syntax: account [account name]\n", ch );
        interpret( banker, ( char * ) "say if you need help type &WHELP BANK&D." );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    bank = find_bank( arg1 );

    if ( !str_cmp( arg3, "create" ) ) {
        if ( strlen( arg1 ) < 4 ) {
            send_to_char( "Account name must be at least 4 characters.\r\n", ch );
            return;
        }

        if ( strlen( arg2 ) < 5 ) {
            send_to_char( "Invalid Password.  Must be at least 5 characters in length.\r\n", ch );
            return;
        }
        if ( arg2[0] == '!' ) {
            send_to_char( "Password cannot begin with the '!' character.\r\n", ch );
            return;
        }

        if ( bank ) {
            send_to_char( "There is already an account with that name!\r\n", ch );
            return;
        }

        if ( ( currtime - ch->pcdata->lastaccountcreated ) < 3600 ) {
            send_to_char
                ( "Please wait at least one hour from previous creation time to make a new account.\r\n",
                  ch );
            return;
        }

        pwdnew = crypt( arg2, arg1 );
        for ( p = pwdnew; *p != '\0'; p++ ) {
            if ( *p == '~' ) {
                send_to_char( "Password not acceptable, try again.\r\n", ch );
                return;
            }
        }

        CREATE( bank, BANK_DATA, 1 );
        bank->lastused = current_time;
        bank->name = STRALLOC( arg1 );
        bank->password = STRALLOC( pwdnew );
        bank->bronze = 0;
        bank->copper = 0;
        bank->gold = 0;
        bank->silver = 0;
        add_bank( bank );
        save_bank(  );
        ch->pcdata->lastaccountcreated = currtime;
        save_char_obj( ch );
        saving_char = NULL;
        send_to_char( "Your account has been added.\r\n", ch );
        return;
    }
    else if ( !str_cmp( arg3, "delete" ) ) {
        if ( !bank ) {
            send_to_char( "There is no account with that name!\r\n", ch );
            return;
        }

        if ( strcmp( crypt( arg2, bank->password ), bank->password ) ) {
            send_to_char( "Invalid password.\r\n", ch );
            return;
        }

        GET_MONEY( ch, CURR_GOLD ) += bank->gold;
        GET_MONEY( ch, CURR_SILVER ) += bank->silver;
        GET_MONEY( ch, CURR_BRONZE ) += bank->bronze;
        GET_MONEY( ch, CURR_COPPER ) += bank->copper;
        ch_printf( ch, "Deleting... (%s)\r\n", bank->name );
        free_bank_to_chars( bank );
        unlink_bank( bank );
        free_bank( bank );
        save_bank(  );
        ch->pcdata->lastaccountcreated = 0;
        save_char_obj( ch );
        saving_char = NULL;
        send_to_char( "Your account has successfully been deleted.\r\n", ch );
        return;
    }

    if ( !bank ) {
        send_to_char( "There is no account by that name!\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "" ) ) {
        if ( ch->pcdata->bank == NULL ) {
            send_to_char( "You don't have any bank account open now.\r\n", ch );
            return;
        }
        snprintf( buf, MSL, "You have closed the account %s.\r\n", ch->pcdata->bank->name );
        send_to_char( buf, ch );
        ch->pcdata->bank = NULL;
        return;
    }

    if ( strcmp( crypt( arg2, bank->password ), bank->password ) ) {
        send_to_char( "Invalid password.\r\n", ch );
        return;
    }

    snprintf( buf, MSL, "You have opened the account %s.\r\n", bank->name );
    send_to_char( buf, ch );
    ch->pcdata->bank = bank;
}

void do_deposit( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    CHAR_DATA              *banker;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    int                     type = DEFAULT_CURR;
    int                     amount = 0,
        x;
    bool                    dall = FALSE,
        ssave = FALSE;

    /*
     * arg1 == amount
     * arg2 == currency
     */
    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        snprintf( buf, MSL, "say Sorry, %s, we don't do business with mobs.", ch->name );
        interpret( banker, buf );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Syntax: deposit [amount] [currency]\n", ch );
        interpret( banker, ( char * ) "say if you need help type &WHELP BANK&D." );
        return;
    }

    if ( !ch->pcdata->bank ) {
        send_to_char( "You don't have any bank account open now.\r\n", ch );
        return;
    }

    /*
     * Just to make sure that we are retrieving the latest bank info, not relying on the
     * stale bank info in pcdata struct 
     */
    bank = find_bank( ch->pcdata->bank->name );
    if ( !bank ) {
        send_to_char( "There is no account by that name!\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1 && !str_cmp( arg1, "all" ) )
        dall = TRUE;

    if ( dall && ( !arg2 || arg2[0] == '\0' ) ) {
        for ( x = 0; x < MAX_CURR_TYPE; x++ ) {
            if ( GET_MONEY( ch, x ) > 0 ) {
                ssave = TRUE;
                amount = GET_MONEY( ch, x );

                GET_MONEY( ch, x ) -= amount;
                ch_printf( ch, "You put %d %s coins in the bank.\r\n", amount, curr_types[x] );
                if ( x == CURR_BRONZE ) {
                    bank->bronze += amount;
                    ch_printf( ch, "This brings your account's bronze balance to %d.\r\n",
                               bank->bronze );
                }
                else if ( x == CURR_COPPER ) {
                    bank->copper += amount;
                    ch_printf( ch, "This brings your account's copper balance to %d.\r\n",
                               bank->copper );
                }
                else if ( x == CURR_GOLD ) {
                    bank->gold += amount;
                    ch_printf( ch, "This brings your account's gold balance to %d.\r\n",
                               bank->gold );
                }
                else if ( x == CURR_SILVER ) {
                    bank->silver += amount;
                    ch_printf( ch, "This brings your account's silver balance to %d.\r\n",
                               bank->silver );
                }
            }
        }
        if ( ssave ) {
            save_char_obj( ch );
            bank->lastused = current_time;
            save_bank(  );
        }
        return;
    }

    if ( !dall && !is_number( arg1 ) ) {
        send_to_char( "How much do you wish to deposit.\r\n", ch );
        return;
    }

    if ( !dall )
        amount = atoi( arg1 );

    if ( arg2 )
        type = get_currency_type( arg2 );

    if ( dall )
        amount = GET_MONEY( ch, type );

    if ( amount <= 0 ) {
        send_to_char( "You can't do that.\r\n", ch );
        return;
    }

    if ( amount > GET_MONEY( ch, type ) ) {
        send_to_char( "You don't have that much.\r\n", ch );
        return;
    }

    if ( type != CURR_BRONZE && type != CURR_COPPER && type != CURR_GOLD && type != CURR_SILVER ) {
        send_to_char( "No such currency.\r\n", ch );
        return;
    }

    GET_MONEY( ch, type ) -= amount;
    ch_printf( ch, "You put %d %s coins in the bank.\r\n", amount, curr_types[type] );

    if ( type == CURR_BRONZE ) {
        bank->bronze += amount;
        ch_printf( ch, "This brings your account's bronze balance to %d.\r\n", bank->bronze );
    }
    else if ( type == CURR_COPPER ) {
        bank->copper += amount;
        ch_printf( ch, "This brings your account's copper balance to %d.\r\n", bank->copper );
    }
    else if ( type == CURR_GOLD ) {
        bank->gold += amount;
        ch_printf( ch, "This brings your account's gold balance to %d.\r\n", bank->gold );
    }
    else if ( type == CURR_SILVER ) {
        bank->silver += amount;
        ch_printf( ch, "This brings your account's silver balance to %d.\r\n", bank->silver );
    }
    save_char_obj( ch );
    bank->lastused = current_time;
    save_bank(  );
}

void do_withdraw( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    CHAR_DATA              *banker;

    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    int                     type = DEFAULT_CURR;
    int                     amount = 0;

    /*
     * arg1 == amount
     * arg2 == currency
     */

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        snprintf( buf, MSL, "say Sorry, %s, we don't do business with mobs.", ch->name );
        interpret( banker, buf );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Syntax: withdraw [amount] [currency]\n", ch );
        interpret( banker, ( char * ) "say if you need help type &WHELP BANK&D." );
        return;
    }

    if ( ch->pcdata->bank == NULL ) {
        send_to_char( "You don't have any bank account open now.\r\n", ch );
        return;
    }

    /*
     * Just to make sure that we are retrieving the latest bank info, not relying on the
     * stale bank info in pcdata struct 
     */
    bank = find_bank( ch->pcdata->bank->name );

    if ( !bank ) {
        send_to_char( "There is no account by that name!\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !str_cmp( arg1, "all" ) ) {
        amount = -1;
    }
    else if ( !is_number( arg1 ) ) {
        send_to_char( "You can only withdraw coins.\r\n", ch );
        return;
    }
    else {
        amount = atoi( arg1 );
    }
    if ( arg1 && arg2 )
        type = get_currency_type( arg2 );
    if ( type == CURR_NONE ) {
        send_to_char( "You don't have any of that kind of coin.\r\n", ch );
        return;
    }
    if ( amount <= 0 ) {
        send_to_char( "You can't do that.\r\n", ch );
        return;
    }

    if ( money_weight( amount, type ) + ch->carry_weight >= can_carry_w( ch ) ) {
        send_to_char( "You can't carry that much.\r\n", ch );
        return;
    }

    if ( type == CURR_BRONZE ) {
        if ( amount > bank->bronze ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->bronze -= amount;
        ch_printf( ch, "This brings your account bronze balance to %d.\r\n", bank->bronze );
    }
    else if ( type == CURR_COPPER ) {
        if ( amount > bank->copper ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->copper -= amount;
        ch_printf( ch, "This brings your account copper balance to %d.\r\n", bank->copper );
    }
    else if ( type == CURR_GOLD ) {
        if ( amount > bank->gold ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->gold -= amount;
        ch_printf( ch, "This brings your account gold balance to %d.\r\n", bank->gold );
    }
    else if ( type == CURR_SILVER ) {
        if ( amount > bank->silver ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->silver -= amount;
        ch_printf( ch, "This brings your account silver balance to %d.\r\n", bank->silver );
    }
    else {
        send_to_char( "No such currency.\r\n", ch );
        return;
    }
    GET_MONEY( ch, type ) += amount;
    ch_printf( ch, "You took %d %s coins from the bank.\r\n", amount, curr_types[type] );
    bank->lastused = current_time;
    save_bank(  );
    save_char_obj( ch );
    return;
}

void do_balance( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    CHAR_DATA              *banker;
    char                    buf[MAX_STRING_LENGTH];

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        snprintf( buf, MSL, "say Sorry, %s, we don't do business with mobs.", ch->name );
        interpret( banker, buf );
        return;
    }

    if ( ch->pcdata->bank == NULL ) {
        send_to_char( "You don't have any bank account open now.\r\n", ch );
        return;
    }

    /*
     * Just to make sure that we are retrieving the latest bank info, not relying on the
     * stale bank info in pcdata struct 
     */
    bank = find_bank( ch->pcdata->bank->name );
    if ( !bank ) {
        send_to_char( "There is no account by that name!\r\n", ch );
        return;
    }

    ch_printf( ch, "&CFor the %s bank account the following funds are available:\r\n", bank->name );
    ch_printf( ch, "&YThere is %d gold.\r\n", bank->gold );
    ch_printf( ch, "&YThere is %d silver.\r\n", bank->silver );
    ch_printf( ch, "&YThere is %d bronze.\r\n", bank->bronze );
    ch_printf( ch, "&YThere is %d copper.\r\n", bank->copper );
}

void do_send( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    BANK_DATA              *victim_bank;
    CHAR_DATA              *banker;
    char                    buf[MAX_STRING_LENGTH];
    int                     type = DEFAULT_CURR;
    int                     amount = 0;

    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    arg3[MAX_INPUT_LENGTH];

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        snprintf( buf, MSL, "say Sorry, %s, we don't do business with mobs.", ch->name );
        interpret( banker, buf );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Syntax: send [amount] [currency] [destination account]\r\n\r\n", ch );
        interpret( banker, ( char * ) "say if you need help type &WHELP BANK&D." );
        return;
    }

    if ( ch->pcdata->bank == NULL ) {
        send_to_char( "You don't have any bank account open now.\r\n", ch );
        return;
    }

    /*
     * Just to make sure that we are retrieving the latest bank info,not relying on the
     * stale bank info in pcdata struct 
     */
    bank = find_bank( ch->pcdata->bank->name );

    if ( !bank ) {
        send_to_char( "There is no account by that name!\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( !str_cmp( arg1, "all" ) ) {
        amount = -1;
    }
    else if ( !is_number( arg1 ) ) {
        send_to_char( "You can only withdraw coins.\r\n", ch );
        return;
    }
    else {
        amount = atoi( arg1 );
    }
    if ( arg1 && arg2 )
        type = get_currency_type( arg2 );
    if ( type == CURR_NONE ) {
        send_to_char( "You don't have any of that kind of coin.\r\n", ch );
        return;
    }
    if ( amount <= 0 ) {
        send_to_char( "You can't do that.\r\n", ch );
        return;
    }
    victim_bank = find_bank( arg3 );

    if ( !victim_bank ) {
        sprintf( buf, "%s There is no account by that name here.", ch->name );
        do_tell( banker, buf );
        return;
    }
    if ( type == CURR_BRONZE ) {
        if ( amount > bank->bronze ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }

        bank->bronze -= amount;
        victim_bank->bronze += amount;
    }
    else if ( type == CURR_COPPER ) {
        if ( amount > bank->copper ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }

        bank->copper -= amount;
        victim_bank->copper += amount;
    }
    else if ( type == CURR_GOLD ) {
        if ( amount > bank->gold ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->gold -= amount;
        victim_bank->gold += amount;
    }
    else if ( type == CURR_SILVER ) {
        if ( amount > bank->silver ) {
            ch_printf( ch, "You don't have that much %s in the bank.\r\n", curr_types[type] );
            return;
        }
        bank->silver -= amount;
        victim_bank->silver += amount;
    }
    ch_printf( ch, "You send %d %s coins from your account to theirs.\r\n", amount,
               curr_types[type] );
    bank->lastused = current_time;
    save_bank(  );
    return;
}

/*
 * Syntax: bankedit [delete/password/amount] [name] [new password or amount]
 */

void do_bankedit( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    int                     type = DEFAULT_CURR;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    arg3[MAX_INPUT_LENGTH];
    char                    arg4[MIL];
    char                    arg5[MSL];

    int                     amount;

    if ( IS_NPC( ch ) ) {
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char
            ( "Syntax:\tbankedit [delete/password/amount] [name] [new password or amount] [currency] [add/subtract]\r\n",
              ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    bank = find_bank( arg2 );

    if ( !bank ) {
        send_to_char( "There is no account with that name!\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "delete" ) ) {
        ch_printf( ch, "Deleting... (%s)\r\n", bank->name );
        free_bank_to_chars( bank );
        unlink_bank( bank );
        free_bank( bank );
        save_bank(  );
        send_to_char( "The account has successfully been deleted.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg3 );

    if ( !str_cmp( arg1, "password" ) ) {
        char                   *pwdnew,
                               *p;

        if ( arg3 == '\0' ) {
            send_to_char( "You have to have something for a password.\r\n", ch );
            return;
        }
        if ( strlen( arg3 ) < 5 ) {
            send_to_char( "Invalid Password.  Must be at least 5 characters in length.\r\n", ch );
            return;
        }
        if ( arg3[0] == '!' ) {
            send_to_char( "Password cannot begin with the '!' character.\r\n", ch );
            return;
        }

        pwdnew = crypt( arg3, bank->name );
        for ( p = pwdnew; *p != '\0'; p++ ) {
            if ( *p == '~' ) {
                send_to_char( "Password not acceptable, try again.\r\n", ch );
                return;
            }
        }

        STRFREE( bank->password );
        bank->password = STRALLOC( pwdnew );
        bank->lastused = current_time;
        save_bank(  );
        ch_printf( ch, "The password has been changed to %s.\r\n", arg3 );
        return;
    }
    argument = one_argument( argument, arg4 );
    if ( !str_cmp( arg1, "amount" ) ) {
        amount = atoi( arg3 );

        if ( arg3 && arg4 )
            type = get_currency_type( arg4 );

        if ( amount < 0 ) {
            send_to_char( "A positive value please.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg5, "add" ) ) {
            if ( type == CURR_BRONZE ) {
                bank->bronze += amount;
            }
            if ( type == CURR_COPPER ) {
                bank->copper += amount;
            }
            if ( type == CURR_GOLD ) {
                bank->gold += amount;
            }
            if ( type == CURR_SILVER ) {
                bank->silver += amount;
            }
        }

        if ( !str_cmp( arg5, "subtract" ) ) {
            if ( type == CURR_BRONZE ) {
                bank->bronze -= amount;
            }
            if ( type == CURR_COPPER ) {
                bank->copper -= amount;
            }
            if ( type == CURR_GOLD ) {
                bank->gold -= amount;
            }
            if ( type == CURR_SILVER ) {
                bank->silver -= amount;
            }

        }
        bank->lastused = current_time;
        save_bank(  );
        ch_printf( ch, "The amount has been changed to %d.\r\n", bank->amount );
        return;
    }
}

/*
 *  bankinfo all
 *  bankinfo [name]
 */

void do_bankinfo( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;

    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Syntax: bankinfo all/old\r\nSyntax: bankinfo [name]\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !str_cmp( arg1, "all" ) ) {
        int                     iHash;
        int                     counter = 0;
        BANK_DATA              *bank;

        for ( iHash = 0; iHash < 27; iHash++ ) {
            for ( bank = bank_index[iHash]; bank; bank = bank->next ) {
                counter++;
                ch_printf( ch, "%d. %s %s\r\n", counter,
                           bank->lastused == 0 ? "Never" : c_time( bank->lastused, -1 ),
                           bank->name );
            }
        }
        return;
    }
    else if ( !str_cmp( arg1, "old" ) ) {
        int                     iHash,
                                counter = 0,
            days;
        BANK_DATA              *bank;

        for ( iHash = 0; iHash < 27; iHash++ ) {
            for ( bank = bank_index[iHash]; bank; bank = bank->next ) {
                if ( !bank->lastused )
                    days = 0;
                else {
                    days = ( ( current_time - bank->lastused ) / 86400 );
                    if ( days < 30 )
                        continue;
                }
                ch_printf( ch, "%d. %s %s\r\n", ++counter,
                           days == 0 ? "Never" : c_time( bank->lastused, -1 ), bank->name );
            }
        }
        if ( counter == 0 )
            send_to_char( "All banks have been used in the last 30 days.\r\n", ch );
        return;
    }

    bank = find_bank( arg1 );

    if ( !bank ) {
        send_to_char( "There is no account with that name!\r\n", ch );
        return;
    }

    ch_printf( ch, "Name: %s\r\nPassword: %s\r\nLastUsed: %d\r\nBalance: %d\r\n", bank->name,
               bank->password, c_time( bank->lastused, -1 ), bank->amount );
}

void free_bank_to_chars( BANK_DATA * bank )
{
    CHAR_DATA              *ch;

    if ( !bank )
        return;

    for ( ch = first_char; ch; ch = ch->next ) {
        if ( !ch || !ch->pcdata || !ch->pcdata->bank )
            continue;
        if ( ch->pcdata->bank != bank )
            continue;
        ch_printf( ch, "%s account no longer exist.\r\n", bank->name );
        ch->pcdata->bank = NULL;
    }
}
