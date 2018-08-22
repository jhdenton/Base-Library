/*******************************************************************************
* File Name: Pushbutton.c  
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
#include "Pushbutton_di_api.h"
#include "Pushbutton_InPin.h"


#define INPUT_ACTIVE    ((0) ? 1 : 0)
#define INPUT_INACTIVE  (1 - INPUT_ACTIVE)



static OS_task_t this;

static Pushbutton_action_t present_state = Pushbutton_DEACTIVATED;
static bool is_active_during_sleep = false;
static bool is_awake = true;
static Pushbutton_callback activation_callback = NULL;
static Pushbutton_callback deactivation_callback = NULL;
static Pushbutton_action_t history[2];



static void Pushbutton_Handle (OS_timestamp_t ts_now);
void Pushbutton_Sleep (void);
void Pushbutton_WakeUp (void);

static void set_drive_mode (void)
{
    #if (2 == 2)
        { Pushbutton_InPin_SetDriveMode(Pushbutton_InPin_DM_RES_UP); }
    #elif (3 == 2)
        { Pushbutton_InPin_SetDriveMode(Pushbutton_InPin_DM_RES_DOWN); }
    #else
        { Pushbutton_InPin_SetDriveMode(Pushbutton_InPin_DM_DIG_HIZ); }
    #endif
}




void Pushbutton_Start (Pushbutton_callback activate_callback,
                             Pushbutton_callback deactivate_callback,
                             bool is_active_in_sleep_mode)
{
    set_drive_mode();
    is_active_during_sleep = is_active_in_sleep_mode;
    is_awake = true;
    activation_callback = activate_callback;
    deactivation_callback = deactivate_callback;
    OS_CreateTask(&this, 10,
                                       Pushbutton_Handle,
                                       Pushbutton_Sleep,
                                       Pushbutton_WakeUp);
}


Pushbutton_action_t Pushbutton_Read (void)
{
    return present_state;
}


void Pushbutton_Sleep (void)
{
    if (!is_active_during_sleep)
    {
        is_awake = false;
        Pushbutton_InPin_SetDriveMode(Pushbutton_InPin_DM_DIG_HIZ);
    }
}


void Pushbutton_WakeUp (void)
{
    is_awake = true;
    set_drive_mode();
}



static void Pushbutton_Handle (OS_timestamp_t ts_now)
{
    static uint8 debounce_index = 0;
    uint8 idx;
    uint8 accum;
    Pushbutton_action_t reading;

    (void)ts_now;

    if (is_awake)
    {
        if (INPUT_ACTIVE == Pushbutton_InPin_Read())
        {
            reading = Pushbutton_ACTIVATED;
        }
        else
        {
            reading = Pushbutton_DEACTIVATED;
        }

        history[debounce_index] = reading;
        if (reading != present_state)
        {
            accum = 0;
            for (idx = 0; idx < 2; idx++)
            {
                if (history[idx] == reading)
                {
                    accum += 1;
                }
            }
            if (accum >= 2)
            {
                present_state = reading;
                if ((Pushbutton_ACTIVATED == present_state) && (NULL != activation_callback))
                {
                    activation_callback();
                }
                if ((Pushbutton_DEACTIVATED == present_state) && (NULL != deactivation_callback))
                {
                    deactivation_callback();
                }
            }
        }

        debounce_index = (debounce_index + 1) % 2;
    }
}



/* [] END OF FILE */
