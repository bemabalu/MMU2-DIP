
#include "Buttons.h"
#include "shr16.h"
#include "trinamic.h"
#include "mmctl.h"
#include "stepper.h"
#include "permanent_storage.h"
#include "main.h"
#include "motion.h"
#include "adc.h"

void settings_bowden_length();

//!  @brief Select filament for bowden length calibration
//!
//! Filaments are selected by left and right buttons, calibration is activated by middle button.
//! Park position (one behind last filament) can be also selected.
//! Activating calibration in park position exits selector.
//!
//! @retval true exit
//! @retval false to be called again
bool settings_select_filament()
{
  manual_extruder_selector();
  delay(50);
  if(Btn::middle == buttonPressed())
  {
    shr16_set_led(2 << 2 * (4 - active_extruder));
    delay(100);
    if (Btn::middle == buttonPressed())
    {
      motion_set_idler_selector(active_extruder);
      if (active_extruder < 5) settings_bowden_length();
      else
      {
        select_extruder(0);
        return true;
      }
    }
  }
  return false;
}

//!  @brief Show setup menu
//!
//! Items are selected by left and right buttons, activated by middle button.
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 11 | 00 | 00 | 00 | 01 | initial state, no action
//! 11 | 00 | 00 | 01 | 00 | setup bowden length
//! 11 | 00 | 01 | 00 | 00 | erase EEPROM if unlocked
//! 11 | 01 | 00 | 00 | 00 | unlock EEPROM erase
//! 11 | 00 | 00 | 00 | 00 | exit setup menu
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//!
//! @retval true continue
//! @retval false exit
bool setupMenu()
{
  static bool onEnter = true;
  if (onEnter)
  {
    lcd("Setup");
    shr16_set_led(0x000);
    delay(200);
    shr16_set_led(0x2aa);
    delay(1200);
    shr16_set_led(0x000);
    delay(600);
    onEnter = false;
  }

  static int _menu = 0;
  bool _exit = false;
  static bool eraseLocked = true;
  static bool inBowdenCalibration = false;

  if (inBowdenCalibration)
  {
    lcd("Bowden Cali active");
    _exit = settings_select_filament();
  }
  else
  {
    shr16_set_led(1 << 2 * 4);
    delay(1);
    shr16_set_led(2 << 2 * 4);
    delay(1);
    shr16_set_led(2 << 2 * _menu);
    delay(1);
    switch (buttonPressed())
    {
    case Btn::right:
        if (_menu > 0) { _menu--; delay(500); }
        break;
    case Btn::middle:
        switch (_menu)
        {
        case 0:
            break;
        case 1:
            inBowdenCalibration = true;
            break;
        case 2:
            if (!eraseLocked)
            {
                eepromEraseAll();
                _exit = true;
            }
            break;
        case 3: //unlock erase
            eraseLocked = false;
            break;
        case 4: // exit menu
            _exit = true;
            lcd(" ");
            break;
        }
        break;
    case Btn::left:
        if (_menu < 4) { _menu++; delay(500); }
        break;
    default:
        break;
    }
    switch (_menu){
      case 0:
        lcd("Setup Menu");
        break;
      case 1:
        lcd("Bowden Cali");
        break;
      case 2:
        lcd("Eprom Erase");
        break;
      case 3:
        lcd("Eprom unlock");
        break;
      case 4:
        lcd("exit Setup");
        break;
    }
  }

  if (_exit)
  {
    shr16_set_led(0x000);
    delay(400);
    shr16_set_led(0x2aa);
    delay(400);
    shr16_set_led(0x000);
    delay(400);

    shr16_set_led(0x000);
    shr16_set_led(1 << 2 * (4 - active_extruder));

    return false;
  }
  return true;
}

//! @brief Set bowden length
//!
//! button | action
//! ------ | ------
//! left   | increase bowden length / feed more filament
//! right  | decrease bowden length / feed less filament
//! middle | store bowden length to EEPROM and exit
//!
//! This state is indicated by following LED pattern:
//!
//! RG | RG | RG | RG | RG
//! -- | -- | -- | -- | --
//! bb | 00 | 00 | 0b | 00
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
//!
void settings_bowden_length()
{
  // load filament above Bondtech gears to check correct length of bowden tube
  if (!isFilamentLoaded)    
  {
    BowdenLength bowdenLength;
    load_filament_withSensor(false);

    uint32_t saved_millis=millis();
    bool button_active = false;
    do
    {
      switch (buttonPressed())
      {
      case Btn::right:
					if (!button_active || (((millis() - saved_millis) > 1000) && button_active)) {
						if (bowdenLength.decrease()) {
							set_pulley_dir_pull();

							for(auto i = bowdenLength.stepSize; i > 0; --i)
							{
								delayMicroseconds(1200);
								do_pulley_step();
							}
						}
					}
					button_active = true;
					break;
      case Btn::left:
					if (!button_active || (((millis() - saved_millis) > 1000) && button_active)) {
						if(bowdenLength.increase()) {
							set_pulley_dir_push();

							for(auto i = bowdenLength.stepSize; i > 0; --i)
							{
								delayMicroseconds(1200);
								do_pulley_step();
							}
						}
					}
					button_active = true;
					break; 
      case Btn::middle:
          bowdenLength.~BowdenLength();
          break;
      default:
					button_active = false;
					saved_millis = millis();
					break;
      }

      shr16_set_led(1 << 2 * 4);
      delay(10);
      shr16_set_led(2 << 2 * 4);
      delay(10);
      shr16_set_led(2 << 2 * 1);
      delay(50);


    } while (buttonPressed() != Btn::middle);

    unload_filament_withSensor();
  }
}

//! @brief Is button pressed?
//!
//! @return button pressed
Btn buttonPressed()
{
  if (adc_val < 30u) return Btn::right;
  if (adc_val > 310u && adc_val < 560u) return Btn::middle;
  if (adc_val > 600u && adc_val < 1000u) return Btn::left;
  return Btn::none;
}

//! @brief Was button clicked?
//!
//! If is buttonPressed, waits until it is released.
//! @return button clicked
Btn buttonClicked()
{
  Btn retVal = buttonPressed();
  if (retVal != Btn::none)
  {
    while (buttonPressed() != Btn::none);
  }
  return retVal;
}
