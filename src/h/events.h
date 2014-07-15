#define WOLLA(fun)   void fun (void)
#define CMDF(fun)    void fun (void)

/* events.c */
void dailyevents        args( ( void ) );
void happymoment        args( ( void ) );
void merrychristmas     args( ( void ) );
void event_doubleexp    args( ( void ) );
void event_happyhour    args( ( void ) );
void event_plauge       args( ( void ) );
void random_spread      args( ( void ) );
void printf_to_char     args( ( CHAR_DATA *ch, const char *fmt, ... ) );

#define OBJ_VNUM_DEMON_TOKEN_FOUND   26
#define OBJ_VNUM_ANGEL_TOKEN_FOUND   27
#define OBJ_VNUM_DEMON_TOKEN_BOUGHT  32
#define OBJ_VNUM_ANGEL_TOKEN_BOUGHT  33
#define OBJ_VNUM_CHRISTMAS_COOKIE    35
#define OBJ_VNUM_QUEST_PILL	     50

bool chance             args( ( int num ) );
bool bigchance          args( ( int num ) );

#ifndef GLOBALS_H
#define GLOBALS_H
#ifdef IN_DB_C
#ifdef GLOBAL
#undef GLOBAL
#endif
#ifdef GLOBAL_DEF
#undef GLOABL_DEF
#endif
#define GLOBAL(str) str
#define GLOBAL_DEF(str, def) str = def

#else
#ifdef GLOBAL
#undef GLOBAL
#endif
#ifdef GLOBAL_DEF
#undef GLOABL_DEF
#endif
#define GLOBAL(str) extern str
#define GLOBAL_DEF(str, def) extern str
#endif

GLOBAL_DEF( DESCRIPTOR_DATA *descriptor_list, NULL );
#endif
