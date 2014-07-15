#define DEFAULT_BOARD 0
#define PERSONAL_BOARD 1
#define BOARD_NOTFOUND -1
struct global_board_data
{
    const char             *short_name;
    const char             *long_name;
    short                   read_level;
    short                   write_level;
    const char             *names;
    int                     force_type;
    int                     purge_days;
    NOTE_DATA              *note_first;
};
extern GLOBAL_BOARD_DATA boards[MAX_BOARD];
