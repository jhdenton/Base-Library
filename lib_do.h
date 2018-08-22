/*******************************************************************************
* File Name: BlueLED.h  
* Version 0.0
*
* Description:
*  This file contains Pin function prototypes and register defines
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_BlueLED_H) /* CY_PINS_BlueLED_H */
#define CY_PINS_BlueLED_H

#include "cytypes.h"
#include "cyfitter.h"
#include <stdbool.h>
#include "OS_core_api.h"


/***************************************
*     Data Struct Definitions
***************************************/


/***************************************
*        Function Prototypes             
***************************************/

void BlueLED_Start (bool is_active_in_sleep_mode);
void BlueLED_Sleep (void);
void BlueLED_WakeUp (void);
void BlueLED_On(void);
void BlueLED_Off(void);
void BlueLED_Pulsing(OS_timestamp_t on_time, OS_timestamp_t off_time);
void BlueLED_OneShot(OS_timestamp_t on_time);
uint8 BlueLED_Read (void);


#endif /* CY_PINS_BlueLED_H */

/* [] END OF FILE */
