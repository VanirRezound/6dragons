/************************************************************************
 *Author: Dustan Gunn, a.k.a Taon                                       *
 *Email:  itztaon@aol.com                                               * 
 *Code Status: Just begin, most basic outline still in brainstorming,   *
 *or on paper.                                                          *
 *                                                                      *  
 *Special Information: This code was wrote for use on 6dragons mud. The *
 *code itself still remains the soul property of the Author. The header *
 *must remain with the code at all times or Author retains the right to *
 *revoke the use of the code.                                           *
 *                                                                      *
 *Note: This isnt the final header, just a placement header until I get *
 *my header written how I want it. -Taon                                *   
 ************************************************************************
 *                           FAITH SYSTEM MODULE                        * 
 ***********************************************************************/

#include "h/mud.h"

void display_faith( CHAR_DATA *ch )
{

    if ( ch->Class == CLASS_HELLSPAWN || ch->Class == CLASS_SHADOWKNIGHT
         || ch->Class == CLASS_VAMPIRE ) {
        if ( ch->faith < 20 )
            send_to_char( "Ye of little faith!\r\n", ch );
        else if ( ch->faith < 40 )
            send_to_char( "You have some darkened faith!\r\n", ch );
        else if ( ch->faith < 60 )
            send_to_char( "A foul imprint is upon your soul.\r\n", ch );
        else if ( ch->faith < 80 )
            send_to_char( "You have the blackened heart of the defiler!\r\n", ch );
        else
            send_to_char( "You're so devoted you are known as the Soul-less!\r\n", ch );
    }
    else {
        if ( ch->faith < 20 )
            send_to_char( "Ye of little faith!\r\n", ch );
        else if ( ch->faith < 40 )
            send_to_char( "You have some faith.\r\n", ch );
        else if ( ch->faith < 60 )
            send_to_char( "The gods smile upon you.\r\n", ch );
        else if ( ch->faith < 80 )
            send_to_char( "You have an incredible amount of faith!\r\n", ch );
        else
            send_to_char( "You're a champion of the Heavens!\r\n", ch );
    }

    return;
}

//command will return current faith information.-Taon
void do_faith( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;
    if ( ch->faith < 0 || ch->faith > 100 ) {
        send_to_char( "Your faith is currently out of bounds.\r\n ", ch );
        bug( "[%s's] faith is currently out of bounds.", ch->name );
        return;
    }

    send_to_char( "Faith: ", ch );
    display_faith( ch );
    return;
/*

  Sorry Taon, quick pull here so I can put it in do_status too - at least until it properly becomes an autoskill! 

   if(ch->Class == CLASS_HELLSPAWN || ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_VAMPIRE)
   {
      if(ch->faith < 20)
         send_to_char("Ye of little faith!\r\n",ch);
      else if(ch->faith <  40)
         send_to_char("You have some darkened faith!\r\n",ch);
      else if(ch->faith <  60)
         send_to_char("A foul imprint is upon your soul.\r\n",ch);
      else if(ch->faith <  80)
         send_to_char("You have the blackened heart of the defiler!\r\n",ch);
      else if(ch->faith >= 80)
         send_to_char("You're so devoted you are known as the Soul-less!\r\n",ch);
   }
   else
   {
      if(ch->faith < 20)
         send_to_char("Ye of little faith!\r\n",ch);
      else if(ch->faith <  40)
         send_to_char("You have some faith.\r\n",ch);
      else if(ch->faith <  60)
         send_to_char("The gods smile upon you.\r\n",ch);
      else if(ch->faith <  80)
         send_to_char("You have an incredible amount of faith!\r\n",ch);
      else if(ch->faith >= 80)
         send_to_char("You're a champion of the Heavens!\r\n",ch);
   }
*/
}

//Function will handle adjustments made to faith,
//will also check all adjustments to make sure
//they never exceed min/max limits. -Taon
void adjust_faith( CHAR_DATA *ch, short adjustment )
{
    if ( IS_NPC( ch ) )
        return;
    if ( ch->pcdata->learned[gsn_faith] <= 0 )
        return;

    ch->faith += adjustment;

    // check min/max limits..
    if ( ch->faith > 100 )
        ch->faith = 100;
    if ( ch->faith < 0 )
        ch->faith = 0;
}

//A skill which will require wood to contruct an altar,
//To assist in prayer, and gaining faith. -Taon
/*
void do_construct_altar(CHAR_DATA * ch, char *argument)
{
  OBJECT_DATA obj;
  short chance;
  **note player constructs differnt types of altars depending on sector.
  Grasslands/Forest - Wooden alter
  Mountains/underground/cave  - Rock altar
  air and underwater cant build there.
 if(IS_NPC(ch))
  return;
 if(ch->position != POS_STANDING)
 {
  send_to_char("You must be standing to contruct an altar.\r\n", ch)
  return;
 }
 if(ch->pcdata->learned[gsn_construct_altar] < 0)
 {
  send_to_char("You wouldnt even know where to begin.\r\n", ch)
  return;
 }

   chance = ch->pcdata->learned[gsn_construct_altar] * number_range(1, 3);
 
 if(get_curr_int(ch) > 20)
   chance += 10;
 if(IS_AFFECTED(ch, AFF_BLIND))
   chance /= 2;

 if(chance > 100)
 {
  **success
  learn_from_success(ch, gsn_construct_altar);
 }
 else
   {
    **failure
    learn_from_failure(ch, gsn_construct_altar);
   }
 return;
}
*/

//Allow a player to leave his inprint in the altar.
//I.E. A wooden altar constructed by ch is here.

/*
void do_mold_altar(CHAR_DATA * ch, char *argument)
{
 return;
}
*/

//Prayer is used by monks to slowly increase their focus level,
//over time. Near future, it will be granted to other classes
//to gain faith. -Taon

//Command Status: Installed, fairly well tested, near completetion.
void do_prayer( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    AFFECT_DATA             af;

    one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Syntax: Prayer start\r\nSyntax: Prayer stop\r\n", ch );
        return;
    }

    if ( !IS_AWAKE( ch ) ) {
        send_to_char( "You cant pray while asleep.", ch );
        return;
    }

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are already meditating!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_PRAYER ) && !str_cmp( arg, "start" ) ) {
        send_to_char( "But you're already praying.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "start" ) ) {
        if ( can_use_skill( ch, number_percent(  ), gsn_prayer ) ) {
            if ( ch->fighting ) {
                send_to_char( "You cannot start a prayer during combat.\r\n", ch );
                return;
            }
            send_to_char( "You close your eyes and begin to pray.\r\n", ch );
            af.type = gsn_prayer;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.level = ch->level;
            af.duration = -1;
            af.bitvector = meb( AFF_PRAYER );
            affect_to_char( ch, &af );
            learn_from_success( ch, gsn_prayer );
        }
        else {
            send_to_char( "You can't seem to concentrate enough to properly pray.\r\n", ch );
            learn_from_failure( ch, gsn_prayer );
        }
        return;
    }

    if ( !str_cmp( arg, "stop" ) ) {
        if ( !IS_AFFECTED( ch, AFF_PRAYER ) ) {
            send_to_char( "But you're not praying.\r\n", ch );
            return;
        }

        if ( ch->position != POS_STANDING )
            set_position( ch, POS_STANDING );

        send_to_char( "You come to an end in your prayer and slowly open your eyes.\r\n", ch );
        affect_strip( ch, gsn_prayer );
        xREMOVE_BIT( ch->affected_by, AFF_PRAYER );
        return;
    }
}
