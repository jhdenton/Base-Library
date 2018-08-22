/*******************************************************************************
* File Name: BlueLED.c  
* Version 0.0
*
* Description:
*  This file contains API to enable firmware control of a Pins component.
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "BlueLED_do_api.h"
#include "BlueLED_OutPin.h"

#if 0
void NULL (uint8 onoff);
#endif


#define TASK_PERIOD_1MS     1
#define OUTPUT_ON   (1)
#define OUTPUT_OFF  (1 - 1)


static const OS_timestamp_t LED_MAX_TIME = 10000u;



typedef enum
{
    LED_STATE_OFF,
    LED_STATE_BLINK_OFF,
    LED_STATE_BLINK_ON,
    LED_STATE_CHIRP,
    LED_STATE_ON
} led_state_t;



static OS_timestamp_t on_target = 0;
static OS_timestamp_t off_target = 0;
static OS_timestamp_t prev_timestamp = 0;
static led_state_t current_state = LED_STATE_OFF;

static OS_task_t this;

static bool is_active_during_sleep = false;
static bool is_awake = true;



static void BlueLED_Handle (OS_timestamp_t ts_now);
void BlueLED_Sleep (void);
void BlueLED_WakeUp (void);






void BlueLED_Start (bool is_active_in_sleep_mode)
{
    is_active_during_sleep = is_active_in_sleep_mode;
    is_awake = true;
    OS_CreateTask(&this, TASK_PERIOD_1MS,
                                       BlueLED_Handle,
                                       BlueLED_Sleep,
                                       BlueLED_WakeUp);
}


void BlueLED_Sleep (void)
{
    if (!is_active_during_sleep)
    {
        is_awake = false;
        BlueLED_OutPin_SetDriveMode(BlueLED_OutPin_DM_DIG_HIZ);
    }
}


void BlueLED_WakeUp (void)
{
    is_awake = true;
    BlueLED_OutPin_SetDriveMode(BlueLED_OutPin_DM_STRONG);
}


void BlueLED_On (void)
{
    current_state = LED_STATE_ON;
    BlueLED_OutPin_Write(OUTPUT_ON);
}


void BlueLED_Off (void)
{
    current_state = LED_STATE_OFF;
    BlueLED_OutPin_Write(OUTPUT_OFF);
}


void BlueLED_Pulsing (OS_timestamp_t on_time, OS_timestamp_t off_time)
{
    on_target = (on_time < LED_MAX_TIME) ? on_time : LED_MAX_TIME;
    off_target = (off_time < LED_MAX_TIME) ? off_time : LED_MAX_TIME;
    prev_timestamp = OS_Get() - off_time;
    current_state = LED_STATE_BLINK_OFF;
    BlueLED_OutPin_Write(OUTPUT_OFF);
}


void BlueLED_OneShot (OS_timestamp_t on_time)
{
    on_target = (on_time < LED_MAX_TIME) ? on_time : LED_MAX_TIME;
    off_target = 0;
    prev_timestamp = OS_Get();
    current_state = LED_STATE_CHIRP;
    BlueLED_OutPin_Write(OUTPUT_ON);
}


static void BlueLED_Handle (OS_timestamp_t ts_now)
{
    OS_timestamp_t delta;
    uint8 output;

    if (is_awake)
    {
        delta = ts_now - prev_timestamp;
        switch (current_state)
        {
            case LED_STATE_BLINK_ON:
            {
                if (delta > on_target)
                {
                    prev_timestamp = ts_now;
                    current_state = LED_STATE_BLINK_OFF;
                    output = OUTPUT_OFF;
                }
                else
                {
                    output = OUTPUT_ON;
                }
            }
            break;

            case LED_STATE_BLINK_OFF:
                if (delta > off_target)
                {
                    prev_timestamp = ts_now;
                    current_state = LED_STATE_BLINK_ON;
                    output = OUTPUT_ON;
                }
                else
                {
                    output = OUTPUT_OFF;
                }
            break;

            case LED_STATE_CHIRP:
                if (delta > on_target)
                {
                    prev_timestamp = ts_now;
                    current_state = LED_STATE_OFF;
                    output = OUTPUT_OFF;
                }
                else
                {
                    output = OUTPUT_ON;
                }
            break;

            case LED_STATE_ON:
                output = OUTPUT_ON;
            break;

            case LED_STATE_OFF:
            default:
                output = OUTPUT_OFF;
            break;
        }

        BlueLED_OutPin_Write(output);
    }
}


/*******************************************************************************
* Function Name: BlueLED_Read
****************************************************************************//**
*
* \brief Reads the associated physical port (pin status register) and masks 
*  the required bits according to the width and bit position of the component
*  instance. 
*
* The pin's status register returns the current logic level present on the 
* physical pin.
*
* \return 
*  The current value for the pins in the component as a right justified number.
*
* \funcusage
*  \snippet BlueLED_SUT.c usage_BlueLED_Read  
*******************************************************************************/
uint8 BlueLED_Read(void)
{
    return (uint8)(BlueLED_OutPin_Read());
}



/* [] END OF FILE */
