/*
 *Code Information: I find this command quite useful, it reveals
 *important information which pertains to the player invoking it.
 *do_status was written by: Taon "Dustan Gunn", for use on 6Dragons
 *mud. This function is still currently under major construction. -Taon
 *Note: This isnt the final information header, just using it to
 *keep those with current shell access a little informed.
 */

#include "h/mud.h"

void                    display_faith( CHAR_DATA *ch );

//I intend on cleaning up the format once finished. -Taon

void do_status( CHAR_DATA *ch, char *argument )
{

    if ( IS_NPC( ch ) )
        return;

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "Can't check status while sleeping!\r\n", ch );
        return;
    }

    send_to_char( "\r\n", ch );
    send_to_char
        ( "\r\n&W-------------------&B[&D &i&W&uStatus&D&D&D &B]&D&W---------------------&D\r\n\r\n",
          ch );

    send_to_char( "&B[&D&WHunger&D&B]&D: ", ch );

    if ( !IS_IMMORTAL( ch ) ) {
        if ( ch->pcdata->condition[COND_FULL] <= 0 )
            send_to_char( "&OYou're STARVING to death!!!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] == 1 )
            send_to_char( "&OYour stomach feels as if it could eat your backbone!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] == 2 )
            send_to_char( "&OYou feel faint from hunger.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] == 3 )
            send_to_char( "&OYou're starting to get hungry.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] >= 4 )
            send_to_char( "&OYou're not hungry.&D\r\n", ch );
    }
    else
        send_to_char( "&OYou're always full!&D\r\n", ch );

    send_to_char( "&B[&D&WThirst&D&B]&D: ", ch );

    if ( !IS_IMMORTAL( ch ) ) {
        if ( ch->pcdata->condition[COND_THIRST] <= 0 )
            send_to_char( "&OYou're dying of thirst!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] == 1 )
            send_to_char( "&OYou really could use a sip of water.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] == 2 )
            send_to_char( "&OYour throat is parched.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] == 3 )
            send_to_char( "&OYou don't feel too thirsty.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] >= 4 )
            send_to_char( "&OYou're not thirsty.&D\r\n", ch );
    }
    else
        send_to_char( "&OYou're never THIRSTY!&D\r\n", ch );

    send_to_char( "&B[&D&WEncumbrance&D&B]&D: ", ch );
    ch_printf( ch, "&O%d/%d&D\r\n", ch->carry_weight, can_carry_w( ch ) );

    send_to_char( "&B[&D&WPosition&D&B]&D: ", ch );

    switch ( ch->position ) {
        case POS_DEAD:
            send_to_char( "&OYou're DEAD!!!&D\r\n", ch );
            break;
        case POS_INCAP:
            send_to_char( "&OYou've been incapitated!&D\r\n", ch );
            break;
        case POS_STUNNED:
            send_to_char( "&OYou're STUNNED!!!&D\r\n", ch );
            break;
        case POS_STANDING:
            send_to_char( "&OYou're standing up.&D\r\n", ch );
            break;
        case POS_MEDITATING:
            send_to_char( "&OYou're in a deep meditation.&D\r\n", ch );
            break;
        case POS_SITTING:
        case POS_RESTING:
            send_to_char( "&OYou're sitting down.&D\r\n", ch );
            break;
        case POS_SLEEPING:
            send_to_char( "&OYou're asleep.&D\r\n", ch );
            break;
        case POS_MOUNTED:
            send_to_char( "&OYou're riding on a mount.&D\r\n", ch );
            break;
        case POS_MORTAL:
            send_to_char( "&OYou're mortally wounded!&D\r\n", ch );
            break;
        case POS_EVASIVE:
            send_to_char( "&OYou're taking evasive action.&D\r\n", ch );
            break;
        case POS_DEFENSIVE:
            send_to_char( "&OYou're taking defensive action.&D\r\n", ch );
            break;
        case POS_AGGRESSIVE:
            send_to_char( "&OYou're taking aggressive action.&D\r\n", ch );
            break;
        case POS_BERSERK:
            send_to_char( "&OYou're &iBERSERKING*D!!&D\r\n", ch );
            break;
        case POS_FIGHTING:
            send_to_char( "&OYou're fighting!&D\r\n", ch );
            break;
        default:
            send_to_char( "&OError: Position out of bounds, for status.&D\r\n", ch );
            bug( "Error: do_status doesn't reconize %s's current position.", ch->name );
            break;
    }

    send_to_char( "&B[&D&WMentalstate&D&B]&D: ", ch );

    // Allow the fall throughs in this switch. -Taon
/* Whoops, think these are backwards! Were from -1 to -10, fixed 16/7/08 Volk */
    switch ( ch->mental_state / 10 ) {
        case -10:
        case -9:
            send_to_pager( "&OYou can barely keep your eyes open.&D\r\n", ch );
            break;
        case -8:
        case -7:
        case -6:
            send_to_pager( "&OYou feel quite sedated.&D\r\n", ch );
            break;
        case -5:
        case -4:
        case -3:
            send_to_pager( "&OYou could use a rest.&D\r\n", ch );
            break;
        case -2:
        case -1:
            send_to_pager( "&OYou feel fine.&D\r\n", ch );
            break;
        case 0:
        case 1:
            send_to_pager( "&OYou feel great.&D\r\n", ch );
            break;
        case 2:
        case 3:
            send_to_pager( "&OYour mind is racing, preventing you from thinking straight.&D\r\n",
                           ch );
            break;
        case 4:
        case 5:
        case 6:
            send_to_pager( "&OYour mind is going 100 miles an hour.&D\r\n", ch );
            break;
        case 7:
        case 8:
            send_to_pager( "&OYou have no idea what is real, and what is not.&D\r\n", ch );
            break;
        case 9:
        case 10:
            send_to_pager( "&O&uYou are a Supreme Entity&D&D.\r\n", ch );
            break;
        default:
            send_to_pager( "&OYou're completely messed up!&D\r\n", ch );
            break;
    }

    if ( ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK || ch->thirdclass == CLASS_MONK ) {
        send_to_char( "&B[&D&WFocus&D&B]&D: ", ch );

        if ( ch->focus_level == 0 )
            send_to_char( "&OYou're unfocused.&D\r\n", ch );
        else if ( ch->focus_level < 10 )
            send_to_char( "&OYou're slightly focused.\r\n", ch );
        else if ( ch->focus_level < 20 )
            send_to_char( "&OYou're somewhat focused.&D\r\n", ch );
        else if ( ch->focus_level < 30 )
            send_to_char( "&OYou're quite focused.&D\r\n", ch );
        else if ( ch->focus_level < 40 )
            send_to_char( "&OYou're highly focused.&D\r\n", ch );
        else if ( ch->focus_level <= 50 )
            send_to_char( "&OYou're very focused.&D\r\n", ch );
        else if ( ch->focus_level > 50 || ch->focus_level < 0 ) {
            send_to_char( "Error: Out of bounds, contact Staff ASAP.\r\n", ch );
            bug( "Error: [adjust = %d] focus_level out of bounds on %s.\r\n", ch->focus_level,
                 ch->name );
        }
    }

    if ( ch->faith > 0 && ch->faith <= 100 ) {
        send_to_char( "&B[&WFaith&B]&D: ", ch );
        display_faith( ch );
    }
/*
    if ( ch->Class == CLASS_ANGEL || ch->Class == CLASS_PRIEST
         || ch->Class == CLASS_CRUSADER )
    {
        send_to_char( "&B[&D&WFaith&D&B]&D: ", ch );

        if ( ch->faith < 20 )
            send_to_char( "&OYee of little faith!&D\r\n", ch );
        else if ( ch->faith < 40 )
            send_to_char( "&OYou have some faith.&D\r\n", ch );
        else if ( ch->faith < 60 )
            send_to_char( "&OThe gods smile upon you.&D\r\n", ch );
        else if ( ch->faith < 80 )
            send_to_char( "&OYou have an incredible amount of faith!&D\r\n", ch );
        else
            send_to_char( "&OYou're a champion of the Heavens!&D\r\n", ch );
    }
    if ( ch->Class == CLASS_HELLSPAWN || ch->Class == CLASS_SHADOWKNIGHT
         || ch->Class == CLASS_VAMPIRE )
    {
        send_to_char( "&B[&D&WFaith&D&B]&D: ", ch );

        if ( ch->faith < 20 )
            send_to_char( "Yee of little faith!\r\n", ch );
        else if ( ch->faith < 40 )
            send_to_char( "You have some darkened faith!\r\n", ch );
        else if ( ch->faith < 60 )
            send_to_char( "A foul imprint is upon your soul.\r\n", ch );
        else if ( ch->faith < 80 )
            send_to_char( "You have the blackened heart of the defiler!\r\n", ch );
        else 
            send_to_char( "You're so devoted you are known as the Soul-less!\r\n", ch );
    }
*/
    if ( ch->quest_curr > 1 )
        ch_printf( ch, "&B[&D&WGlory Points&D&B]&D: &O%d&D\r\n", ch->quest_curr );

    send_to_char( "&W\r\nGeneral Information:&D\r\n", ch );

    if ( ch->move <= ch->max_move / 5 )
        send_to_char( "&OYou need to rest, your body is tiring out.&D\r\n", ch );
    if ( ch->mana <= ch->max_mana / 5 )
        send_to_char( "&OYou feel as if your magic is nearly drained.&D\r\n", ch );
    if ( ch->hit <= ch->max_hit / 5 )
        send_to_char( "&OYou're hurt quite badly.&D\r\n", ch );

    if ( IS_BLOODCLASS( ch ) && ( ch->blood <= ch->max_blood / 6 ) )
        send_to_char( "&OYour body begins to &uwithdraw&D&O from lack of blood.&D\r\n", ch );

    if ( ch->pcdata->condition[COND_DRUNK] > 0 ) {
        if ( ch->pcdata->condition[COND_DRUNK] < 3 )
            send_to_char( "&OYou've had a bit to drink.&D.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] < 5 )
            send_to_char( "&OYou've been drinking.&D.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] < 10 )
            send_to_char( "&OYou are starting to feel &udrunk&D.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] >= 10 )
            send_to_char( "&OYou are &udrunk&D.&D\r\n", ch );
    }

    if ( IS_VAMPIRE( ch ) ) {
        if ( IS_OUTSIDE( ch ) ) {
            switch ( time_info.sunlight ) {
                case SUN_LIGHT:
                    send_to_char( "&YYou're burning to a crisp under the sun.&D\r\n", ch );
                    break;
                case SUN_RISE:
                    send_to_char( "&YYou feel your skin begin to burn in the morning light.&D\r\n",
                                  ch );
                    break;
                case SUN_SET:
                    send_to_char( "&YYou feel your skin burning under the setting sun.&D\r\n", ch );
                    break;
            }
        }
    }

    if ( IS_AFFECTED( ch, AFF_PRAYER ) )
        send_to_char( "&OYou're deep in&i prayer.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SHAPESHIFT ) )
        send_to_char( "&OYour shape-shifted into another shape.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_ANOINT ) )
        send_to_char( "&OYou've been recently anointed.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_POISON ) )
        send_to_char( "&OYou feel as if you've been poisoned.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FURY ) || IS_AFFECTED( ch, AFF_BERSERK ) )
        send_to_char( "&OYou're in a &ubloody rage&D&O.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) )
        send_to_char( "&OYour feet aren't touching the ground.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        send_to_char( "&OYou're charmed.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) )
        send_to_char( "&O&i&uA thick film covers your eyes preventing you from seeing.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_HIDE ) )
        send_to_char( "&OYou seem to be &ihiding&D&O from someone.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_MAIM ) )
        send_to_char( "&OYou've been badly &umaimed&D&O, causing you to bleed freely.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SILENCE ) )
        send_to_char( "&OA magical force prevents you from speaking.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SURREAL_SPEED ) || IS_AFFECTED( ch, AFF_BOOST ) )
        send_to_char( "&OYour body seems to be moving as fast as&i lightning.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        send_to_char( "&O&iYour body is moving at a slow rate..&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_PARALYSIS ) || ch->position == POS_STUNNED )
        send_to_char( "&OYou're suffering from some sort of &i&uparalysis.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FEIGN ) )
        send_to_char( "&OYou lay stiff on the ground, playing possum.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_CURSE ) )
        send_to_char( "&OYou feel as if you're cursed.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_IRON_SKIN ) )
        send_to_char( "&OYour skin is as solid as iron.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_UNSEARING_SKIN ) )
        send_to_char( "&OYour flesh bares resistance to fire.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_ROOT ) )
        send_to_char( "&OYour feet are &urooted&D&O into the ground.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SHIELD ) || IS_AFFECTED( ch, AFF_WARD ) )
        send_to_char( "&OYour surrounded by a magical shield.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_INFRARED ) )
        send_to_char( "&OYour eyes glow bright red.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
        send_to_char( "&OYou can see invisible people and objects.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) )
        send_to_char( "&OYour eyes have been tuned to detect magic.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) )
        send_to_char( "&OYou can see hidden people and objects.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_HEAVENS_BLESS ) )
        send_to_char( "&OYou've been blessed by the heavens.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) )
        send_to_char( "&OYou're protected by the heavens.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_REACTIVE ) )
        send_to_char( "&OYour body heals itself during combat.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
        send_to_char( "&OYour vision is enhanced.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_NOSIGHT ) )
        send_to_char( "&OYou can see without your eyes.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_ICESHIELD ) )
        send_to_char( "&OYour surrounded by a shield of ice.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
        send_to_char( "&OYou're surrounded by a shield of energy.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_FIRESHIELD ) )
        send_to_char( "&OYou're surrounded by a shield of fire.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_RECOIL ) )
        send_to_char( "&OYou're positioned in a coiled up stance.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_KEEN_EYE ) )
        send_to_char( "&OYour eyes are sharply attuned to your surroundings.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_KINETIC ) )
        send_to_char( "&OA barrier of kinetic energy swirls around you.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SUSTAIN_SELF ) )
        send_to_char( "&OYour body is fighting off hunger.\r\n&D", ch );

    if ( ch->hate_level > 0 ) {
        if ( ch->hate_level <= 5 )
            send_to_char( "&OYou're watching out for someone.&D\r\n", ch );
        else if ( ch->hate_level < 20 )
            send_to_char( "&OYou get the feeling someone is after you.\r\n&D", ch );
        else if ( ch->hate_level >= 20 )
            send_to_char( "&OYou're hated amongst your peers.&D\r\n", ch );
    }

    send_to_char( "\r\n&W--------------------------------------------------&D\r\n\r\n", ch );
    return;
}
