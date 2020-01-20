/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         led.h
 Version:             1.0
 Author:              KPA362
 Created:            2017/6/13
 Description:       system led control
 * *******************************************************************************/

#pragma once
#include <utils/Thread.h>
#include <utils/Mutex.h>

class LedControl
{
    public:        
        enum LED_TYPE
        {
        	DEV_LED = 0,
            F_REC_LED,
            R_REC_LED,
        };

        enum SHINING_TYPE
        {
            LONG_LIGHT = 0,
			SHINING_1S_LIGHT,
            SHINING_2S_LIGHT,
            SHINING_5S_LIGHT,
        };

    public:
        LedControl();
        ~LedControl();

        /****************************************************
               * Name:
               *     get()
               * Function:
               *     get LedControl single object
               * Parameter:
               *     input:
               *         none
               *     output:
               *         none
               * Return:
               *     single object
              *****************************************************/
        static LedControl* get();

         /****************************************************
                * Name:
                *     EnableLed(LED_TYPE p_Type, bool p_enable,SHINING_TYPE p_ShinType)
                * Function:
                *     control led turn on/off
                * Parameter:
                *     input:
                *         p_Type:   Led Type(see LED_TYPE enum)
                *         p_enable:    
                *               true: set led on(only led main switch in enable status can affect)
                *               false: set led off
                *         p_ShinType:
                *               LONG_LIGHT: led of all time
                *               SHINING_LIGHT: led first on and then off all the time
                *     output:
                *         none
                * Return:
                *     0: success
               *****************************************************/
        int EnableLed(LED_TYPE p_Type, bool p_enable, SHINING_TYPE p_ShinType=LONG_LIGHT);

        /****************************************************
               * Name:
               *     SetLedMainSwitch(bool p_enable)
               * Function:
               *     set led main switch on/off
               * Parameter:
               *     input:
               *         p_enable:    
               *               true: led main switch enable
               *               false: led main switch disable
               *     output:
               *         none
               * Return:
               *     0: success
              *****************************************************/
        int SetLedMainSwitch(bool p_enable);

       /****************************************************
               * Name:
               *     GetLedMainSwitch(bool &p_enable)
               * Function:
               *     get led main switch status
               * Parameter:
               *     input:
               *        none
               *     output:
               *         p_enable:    
               *               true: led main switch enable
               *               false: led main switch disable
               * Return:
               *     0: success
              *****************************************************/
        int GetLedMainSwitch(bool &p_enable);

        /****************************************************
                * Name:
                *     TurnOffAllLed()
                * Function:
                *     turn off all led
                * Parameter:
                *     input:
                *        none
                *     output:
                *        none
                * Return:
                *     0: success
               *****************************************************/
        int TurnOffAllLed();

    private:
        static pthread_mutex_t m_mutex;
        static LedControl* m_instance;
        bool m_led_main_switch; 
};
