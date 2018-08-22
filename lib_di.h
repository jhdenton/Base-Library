/*******************************************************************************
* File Name: Pushbutton.h  
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

#if !defined(CY_PINS_Pushbutton_H) /* CY_PINS_Pushbutton_H */
#define CY_PINS_Pushbutton_H

#include "cytypes.h"
#include "cyfitter.h"
#include <stdbool.h>
#include "OS_core_api.h"


/***************************************
*     Data Struct Definitions
***************************************/
typedef enum
{
    Pushbutton_DEACTIVATED,
    Pushbutton_ACTIVATED
} Pushbutton_action_t;

typedef void (*Pushbutton_callback)(void);


/***************************************
*        Function Prototypes             
***************************************/

void Pushbutton_Start (Pushbutton_callback activate_callback,
                             Pushbutton_callback deactivate_callback,
                             bool is_active_in_sleep_mode);
void Pushbutton_Stop (void);
void Pushbutton_Sleep (void);
void Pushbutton_WakeUp (void);
Pushbutton_action_t Pushbutton_Read(void);


#endif /* CY_PINS_Pushbutton_H */

/* [] END OF FILE */
