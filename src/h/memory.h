/* New memory stuff */
typedef struct memory_strings_data MEMORY_DATA;
struct memory_strings_data
{
    MEMORY_DATA            *next;
    MEMORY_DATA            *prev;
    char                   *string;
    unsigned long int       links;
    unsigned long int       length;
    unsigned long int       linenumber;
    char                   *filename;
};
extern MEMORY_DATA     *first_memory;
extern MEMORY_DATA     *last_memory;
