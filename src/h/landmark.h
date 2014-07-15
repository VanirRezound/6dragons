typedef struct landmark_data LANDMARK_DATA;
struct landmark_data
{
    LANDMARK_DATA          *next;
    LANDMARK_DATA          *prev;
    struct area_data       *area;
    char                   *name;
    int                     vnum;
};
