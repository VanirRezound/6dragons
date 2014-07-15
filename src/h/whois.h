/****************************************************************************
 *  *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *   *                  / \    |      | /  |\   /| |     | |  \                 *
 *    *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *     *                /-----\  |      | \  |  v  | |     | |  /                 *
 *      *               /       \ |      |  \ |     | +-----+ +-/                  *
 *       ****************************************************************************
 *        * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 *         * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 *          *                                                                          *
 *           * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 *            * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 *             * Grishnakh, Fireblade, and Nivek.                                         *
 *              *                                                                          *
 *               * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                *                                                                          *
 *                 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 *                  * Michael Seifert, and Sebastian Hammer.                                   *
 *                   ****************************************************************************
 *                    *                        Finger and Wizinfo Module                         *
 *                     ****************************************************************************/

#define FINGERCODE                                     /* Do not remove, used to interact 
                                                        * with other snippets! - Samson
                                                        * 10-18-98 */

#ifndef GET_TIME_PLAYED
#define GET_TIME_PLAYED(ch)     (((ch)->played + (current_time - (ch)->logon)) / 3600)
#endif

typedef struct wizinfo_data WIZINFO_DATA;

extern WIZINFO_DATA    *first_wizinfo;
extern WIZINFO_DATA    *last_wizinfo;

struct wizinfo_data
{
    WIZINFO_DATA           *next;
    WIZINFO_DATA           *prev;
    char                   *name;
    char                   *email;
    int                     icq;
    short                   level;
};

void                    build_wizinfo( bool bootup );
