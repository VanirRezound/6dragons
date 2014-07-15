/****************************************************************************
 * AFKMud (c)1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,          *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                      New Name Authorization module                       *
 ****************************************************************************/

#define AUTO_AUTH                                      /* Do not remove, used to interact 
                                                        * with other snippets! - Samson
                                                        * 12-28-98 */

#ifndef MSL
#define MSL MSL
#endif

#ifndef MIL
#define MIL MIL
#endif

typedef struct auth_list AUTH_LIST;                    /* new auth -- Rantic */

/* new auth -- Rantic */
#define NOT_AUTHED(ch) (!IS_NPC(ch) && get_auth_state(ch) != AUTH_AUTHED && IS_SET((ch)->pcdata->flags, PCFLAG_UNAUTHED))
#define NEW_AUTH(ch) (!IS_NPC(ch) && (ch)->level == 1)
#define IS_WAITING_FOR_AUTH(ch) (!IS_NPC(ch) && (ch)->desc && get_auth_state(ch) == AUTH_ONLINE && IS_SET((ch)->pcdata->flags, PCFLAG_UNAUTHED))

/* new auth -- Rantic */
extern AUTH_LIST       *first_auth_name;
extern AUTH_LIST       *last_auth_name;

/* new auth stuff in addit.c */
int                     get_auth_state( CHAR_DATA *ch );
void                    add_to_auth( CHAR_DATA *ch );
void                    remove_from_auth( char *name );
void                    check_auth_state( CHAR_DATA *ch );
AUTH_LIST              *get_auth_name( char *name );
void                    auth_update( void );

/* New auth stuff --Rantic */
typedef enum
{
    AUTH_ONLINE = 0, AUTH_OFFLINE, AUTH_LINK_DEAD, AUTH_CHANGE_NAME,
    AUTH_DENIED, AUTH_AUTHED
} auth_types;

struct auth_list
{
    char                   *name;                      /* Name of character awaiting
                                                        * authorization */
    short                   state;                     /* Current state of authed */
    char                   *authed_by;                 /* Name of immortal who authorized 
                                                        * the name */
    char                   *change_by;                 /* Name of immortal requesting
                                                        * name change */
    char                   *denied_by;                 /* Name of immortal who denied the 
                                                        * name */
    AUTH_LIST              *next;
    AUTH_LIST              *prev;
};
