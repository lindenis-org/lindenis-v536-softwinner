/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         led.cpp
 Version:             1.0
 Author:              KPA362
 Created:            2017/6/13
 Description:       system led control
 * *******************************************************************************/
#include "led.h"
#include "device_model/menu_config_lua.h"
#include "common/app_log.h"
#include "window/window.h"
#include "window/user_msg.h"

using namespace EyeseeLinux;

LedControl* LedControl::m_instance = NULL;
pthread_mutex_t LedControl::m_mutex = PTHREAD_MUTEX_INITIALIZER;

LedControl* LedControl::get()
{
    if ( LedControl::m_instance == NULL )
    {
        pthread_mutex_lock( &LedControl::m_mutex );
        if (LedControl::m_instance == NULL )
        {
            LedControl::m_instance = new LedControl();
        }
        pthread_mutex_unlock( &LedControl::m_mutex );
    }

	return LedControl::m_instance;
}

LedControl::LedControl()
{
    m_led_main_switch = true;
}

LedControl::~LedControl()
{
    m_led_main_switch = false;
}

//system("echo timer > /sys/class/leds/rec_led/trigger");
//system("echo 2000 > /sys/class/leds/rec_led/delay_on");
//system("echo 2000 > /sys/class/leds/rec_led/delay_off");
//system("echo 10 > /sys/class/leds/rec_led/heartbeat_count");
// system("echo 0 > /sys/class/leds/rec_led/brightness");
//system("echo 1 > /sys/class/leds/rec_led/brightness");

int LedControl::EnableLed(LED_TYPE p_Type,bool p_enable,SHINING_TYPE p_ShinType)
{
    db_msg("led_type[%d] enable[%d] main_switch[%d] shinType[%d]\n",p_Type, p_enable,m_led_main_switch,p_ShinType);
    switch( p_Type )
    {
        case DEV_LED:
			if(m_led_main_switch && p_enable){
				if(p_ShinType == LONG_LIGHT){
					system("echo 0 > /sys/class/leds/rec_led/brightness");
				}else if(p_ShinType == SHINING_2S_LIGHT){
					system("echo timer > /sys/class/leds/rec_led/trigger");
					system("echo 2000 > /sys/class/leds/rec_led/delay_on");
					system("echo 10 > /sys/class/leds/rec_led/heartbeat_count");
				}else if(p_ShinType == SHINING_5S_LIGHT){
					system("echo timer > /sys/class/leds/rec_led/trigger");
					system("echo 5000 > /sys/class/leds/rec_led/delay_on");
				}else if(p_ShinType == SHINING_1S_LIGHT){
					system("echo 0 > /sys/class/leds/rec_led/brightness");
				}
			}else{
                if( m_led_main_switch){
					system("echo 1 > /sys/class/leds/rec_led/brightness");
                }
			}
			db_msg("by hero *** EnableLed DEV_LED\n");
            break;
        case F_REC_LED:			
			db_msg("by hero *** EnableLed F_REC_LED\n");
            break;
		case R_REC_LED:		
			db_msg("by hero *** EnableLed R_REC_LED\n");
			break;
        default:
            break;
    }
    return 0;
}

int LedControl::GetLedMainSwitch(bool & p_enable)
{
    p_enable = m_led_main_switch;

    return 0;
}

int LedControl::SetLedMainSwitch(bool p_enable)
{

    if( p_enable )
    {
        m_led_main_switch = p_enable;
        EnableLed(DEV_LED, p_enable,LONG_LIGHT);
    }
    else
    {
        TurnOffAllLed();
        m_led_main_switch = p_enable;
    }

    return 0;
}

int LedControl::TurnOffAllLed()
{
    EnableLed(DEV_LED, false);
    EnableLed(F_REC_LED, false);
	EnableLed(F_REC_LED, false);
    return 0;
}
