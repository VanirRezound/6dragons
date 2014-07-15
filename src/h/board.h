typedef struct note_data NOTE_DATA;
typedef struct board_data BOARD_DATA;

/* Board Types */
typedef enum
{ BOARD_NOTE, BOARD_MAIL } board_types;

/* Data structure for notes. */
struct note_data
{
    NOTE_DATA              *next;
    NOTE_DATA              *prev;
    char                   *sender;
    char                   *date;
    char                   *to_list;
    char                   *subject;
    char                   *yesvotes;
    char                   *novotes;
    char                   *abstentions;
    char                   *text;
    char                    voting;
    time_t                  expire;
    time_t                  date_stamp;
    time_t                  current;
};

struct board_data
{
    BOARD_DATA             *next;
    BOARD_DATA             *prev;
    NOTE_DATA              *first_note;
    NOTE_DATA              *last_note;
    NOTE_DATA              *in_progress;
    char                   *note_file;
    char                   *read_group;
    char                   *post_group;
    char                   *extra_readers;
    char                   *extra_removers;
    char                   *otakemessg;
    char                   *opostmessg;
    char                   *oremovemessg;
    char                   *ocopymessg;
    char                   *olistmessg;
    char                   *postmessg;
    char                   *oreadmessg;
    int                     board_obj;
    int                     type;
    short                   num_posts;
    short                   min_read_level;
    short                   min_post_level;
    short                   min_remove_level;
    short                   max_posts;
    GLOBAL_BOARD_DATA      *board;
    time_t                  last2_note[MAX_BOARD];
};
