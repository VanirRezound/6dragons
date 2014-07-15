/***************************************************************************
 *                                                                         *
 *   .oOOOo.    o                                        o                 *
 *  .O     o   O                             o          O                  *
 *  o          o                                        o                  *
 *  o          O                                        O                  *
 *  o          OoOo.  `OoOo.  .oOo.  'OoOo.  O   .oOo   o   .oOo.  .oOo    *
 *  O          o   o   o      O   o   o   O  o   O      O   OooO'  `Ooo.   *
 *  `o     .o  o   O   O      o   O   O   o  O   o      o   O          O   *
 *   `OoooO'   O   o   o      `OoO'   o   O  o'  `OoO'  Oo  `OoO'  `OoO'   *
 *                                                                         *
 ***************************************************************************
 * - Chronicles Copyright 2001-2003 by Brad Ensley (Orion Elder)           *
 ***************************************************************************
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - News module header                                                    *
 ***************************************************************************/

/*
 * Version - 1    - Original code. Simplistic.
 * Version - 2    - Coverted to use a linked list, and allow news removal.
 * Version - 3    - Converted time slots to use a timestamp. Made it more
 *                  functional, as such.
 * Version - 4    - Added html functionality.
 * Version - 5    - Advanced somewhat html functionality.
 * Version - 6    - Completed HTML functionality.
 * Version - 6a   - Cleaned up the installation instructions, and fixed
 *                  minor bugs (unused variables).
 * Version - 6b   - Changed e-mail addresses to reflect e-mail change,
 *                  Also added a version announcement.
 * Version - 6c   - Updated the install.txt file to reflect all news
 *                  news options.
 * Version - 6d   - Updated the license.txt file to clarify a licensing
 *                  issue brought to my attention.
 * Version - 6e   - Updated the snippet information, and e-mail address.
 * Version - 7    - Updated the snippet to correct faulty fprintf calls.
 * Version - 8    - Added a parameter to news.h to make news.html creation
 *                  an option.
 * Version - 8a   - Added '<string.h>' to the news.c includes.
 * Version - 8b   - Added a news.help file to the .zip.
 * Version - 8.1a - Added the fptof function.
 * Version - 8.1b - Got bored and changed the default news colors.
 * Version - 8.2a - Removed an unnecessary #include.
 * Version - 8.2b - Moved the directory defines for the news into the
 *                  news.h file.
 * Version - 8.3a - Moved the news definitions and information into the
 *                  news.h file.
 * Version - 8.3b - Added the ability to turn on and off the HTML news
 *                  inside the MUD.
 * Version - 8.3c - Added a fix to make the news system compile under the
 *                  Linux OS properly.
 * Version - 8.3d - Changed the colors of the news bar again.
 * Version - 8.4a - Fixed a minor bug with the remove all subfunction.
 *
 * Used for identification, and with the news 'version' call.
 */
#define NEWS_VERSION		"8.40a"

/*
 * News data file. -Orion
 */
#define NEWS_DIR		"./news/"
#define HTML_DIR                "../public_html/"
#define HTML_NEWS_DIR		"./news/"
#define NEWS_FILE		NEWS_DIR "news.dat"
#define HTML_NEWS_FILE		HTML_DIR "news.html"
#define HTML_NEWS_IMAGES	"http://6dragons.org/img/header.gif"

/*
 * Four times default MAX_STRING_LENGTH, to ensure enough space. -Orion
 */
#define MAX_NEWS_LENGTH		(MAX_STRING_LENGTH * 3)
#define MAX_HTML_LENGTH		(MAX_STRING_LENGTH * 6)

/*
 * News color definitions. -Orion
 */
#define AT_BLADE		"&G&w"
#define AT_BASE			"&G&W"
#define AT_HILT			"&G&Y"
#define AT_BALL			"&G&W"
#define AT_TITLE		"&G&W"

#define AT_DATE			"&G&W"
#define AT_SEPARATOR		"&G&Y"
#define AT_NEWS			"&G&w"
#define AT_6D                   "&R&R"
#define AT_TRANS                "&Y&Y"

/*
 * Type definition of the news_data struct. -Orion
 */
typedef struct news_data NEWS_DATA;

/*
 * News show types. -Orion
 */
typedef enum
{
    TYPE_ALL, TYPE_NORMAL, TYPE_LIST_FIRST,
    TYPE_LIST_LAST, TYPE_IMM_LIST, TYPE_SHOW_ONE,
    MAX_SHOW_TYPE
} news_show_types;

/*
 * News types. -Orion
 */
typedef enum
{
    NEWS_NORM, NEWS_GROUP, NEWS_COUNCIL,
    NEWS_CODE, NEWS_IMM,
    MAX_NEWS_TYPE
} news_types;

/*
 * News structure. -Orion Elder
 */
struct news_data
{
    NEWS_DATA              *next;
    NEWS_DATA              *prev;
    int                     day;
    int                     month;
    int                     year;
    char                   *data;
    time_t                  time_stamp;
};

/*
 * The first and last news pointers for the NEWS_DATA linked list. -Orion
 */
extern NEWS_DATA       *first_news;                    /* Orion Elder */
extern NEWS_DATA       *last_news;                     /* Orion Elder */
