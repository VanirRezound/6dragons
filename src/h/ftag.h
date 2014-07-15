typedef struct tag_data TAG_DATA;

struct tag_data
{
    int                     status;
    int                     timer;
    int                     next;
    int                     playing;
};

#define TAG_OFF 0
#define TAG_ISWAIT 1
#define TAG_ISPLAY 2
#define A		  	1
#define B			2
#define C			4
#define D			8
#define E			16
#define TAG_PLAYING		        (A)
#define TAG_FROZEN		        (B)
#define TAG_RED			        (C)
#define TAG_BLUE			(D)
#define TAG_WAITING                     (E)

#define FTAG_WAIT_ROOM          5400
#define FTAG_MIN_VNUM		5401
#define FTAG_MAX_VNUM		5420

/* freeze.c */
void tag_update         args( ( void ) );
void end_tag            args( ( void ) );
void start_tag          args( ( void ) );
void check_team_frozen  args( ( CHAR_DATA *ch ) );
void auto_tag           args( ( void ) );
bool is_tagging         args( ( CHAR_DATA *ch ) );
void tag_channel        args( ( CHAR_DATA *ch, const char *message ) );
