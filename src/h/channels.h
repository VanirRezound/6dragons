/****************************************************************************
 * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                          Dynamic Channel System                          *
 ****************************************************************************/
#define CHANNEL_FILE SYSTEM_DIR "channels.dat"

bool                    local_channel_hook( CHAR_DATA *ch, const char *command, char *argument );
void                    load_mudchannels( void );
void                    to_channel( const char *argument, const char *xchannel, int level );

typedef struct mud_channel MUD_CHANNEL;

extern MUD_CHANNEL     *first_channel;
extern MUD_CHANNEL     *last_channel;

typedef enum
{
    CHAN_GLOBAL, CHAN_ZONE, CHAN_ALLIANCE, CHAN_COUNCIL,
    CHAN_PK, CHAN_LOG, CHAN_ROOM, CHAN_SECRET,
    CHAN_THRONG, CHAN_HALCYON, CHAN_PALEON, CHAN_DAKAR,
    CHAN_FORBIDDEN
} channel_types;

struct mud_channel
{
    MUD_CHANNEL            *next;
    MUD_CHANNEL            *prev;
    char                   *name;
    char                   *history[20][2];            /* Not saved */
    int                     hlevel[20];                /* Not saved */
    int                     hinvis[20];                /* Not saved */
    int                     level;
    int                     type;
    bool                    keephistory;
    bool                    doscramble;
};
