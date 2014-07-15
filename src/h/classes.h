/* Volk: ALL CLASS DEFINES IN HERE!! Had to do this because of multiclass. Stop changing it idiots. */

#define CLASS_NONE        -1                           /* USED DON'T REMOVE! */
#define CLASS_PRIEST       0
#define CLASS_DRUID        1
#define CLASS_MAGE         2
#define CLASS_NECROMANCER  3
#define CLASS_WARRIOR      4
#define CLASS_CRUSADER     5
#define CLASS_THIEF        6
#define CLASS_DRAGONLORD   7
#define CLASS_BLACK        8                           /* 7-7-96 SB */
#define CLASS_GOLD         9
#define CLASS_SILVER       10
#define CLASS_RED          11
#define CLASS_BLUE         12
#define CLASS_HELLSPAWN    13
#define CLASS_ANGEL        14
#define CLASS_VAMPIRE      15
#define CLASS_MONK         16
#define CLASS_BARD         17
#define CLASS_SHADOWKNIGHT 18
#define CLASS_PSIONIC      19
#define CLASS_HERO         20                          // non-dragon remort
#define CLASS_TWOHEADED    21                          // Dragon remort
#define CLASS_BALROG       22                          // Vampire, Hellspawn remort
#define CLASS_SORCERER     23
#define CLASS_ROGUE        24

#define IS_CLASS(ch, name) (ch->Class == name || ch->secondclass == name || ch->thirdclass == name)
#define NOT_FWARRIOR(ch)         ((ch)->Class != 4 && (ch)->Class !=18 && (ch)->Class != 5 && (ch)->Class != 16 && (ch)->Class != 20 )
#define NOT_SWARRIOR(ch)         ((ch)->secondclass != 4 && (ch)->secondclass !=18 && (ch)->secondclass != 5 && (ch)->secondclass != 16 )
#define NOT_TWARRIOR(ch)         ((ch)->thirdclass != 4 && (ch)->thirdclass !=18 && (ch)->thirdclass != 5 && (ch)->thirdclass != 16 )

#define IS_SINGLECLASS(ch)      ((ch)->secondclass == -1 )
#define IS_SECONDCLASS(ch)      ((ch)->secondclass > -1)
#define IS_THIRDCLASS(ch)       ((ch)->thirdclass > -1)

//For remorts. -Taon
#define IS_REMORT(ch) (!IS_NPC(ch)) && ((ch)->Class == CLASS_BALROG || (ch)->Class == CLASS_ROGUE || (ch)->Class == CLASS_SORCERER || (ch)->Class == CLASS_HERO || (ch)->Class == CLASS_TWOHEADED))

#define IS_BLOODCLASS(ch) (!IS_NPC((ch)) && ((ch)->Class == CLASS_VAMPIRE || (ch)->Class == CLASS_HELLSPAWN || (ch)->Class == CLASS_BALROG))
#define IS_MINDFLAYER(ch) (!IS_NPC((ch)) && ((ch)->Class == CLASS_PSIONIC) && ((ch)->race == RACE_MINDFLAYER))
#define IS_DRAGON(ch) ((ch)->race == RACE_DRAGON)
#define IS_VAMPIRE(ch) ((ch)->race == RACE_VAMPIRE && ((ch)->Class == CLASS_VAMPIRE))

/* Volk: Added these for things like gain_mana in update.c and learn_from_success awarding more XP to caster classes.
 * IS_CASTER is a pure caster, while IS_2CASTER is a support caster. */
#define IS_CASTER(ch) ((ch)->Class == CLASS_MAGE || (ch)->Class == CLASS_NECROMANCER || (ch)->Class == CLASS_PSIONIC)
#define IS_2CASTER(ch) ((ch)->Class == CLASS_PRIEST || (ch)->Class == CLASS_DRUID)
