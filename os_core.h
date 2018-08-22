/******************************************************************************
 *  @file OS_api.h
 *
 *  This file is the header file for the core_api.c module.
 */

#ifndef  OS_API_H
#define  OS_API_H

/* ----------------------------------------------------------------------------
 * Dependencies
 * --------------------------------------------------------------------------*/
#include "cytypes.h"
#include <stdbool.h>
#include <stddef.h>


/* ----------------------------------------------------------------------------
 * Public Constant and Macro Definitions
 * --------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------
 * Public Type Definitions
 * --------------------------------------------------------------------------*/
typedef uint16 OS_timestamp_t;

typedef void (*OS_task_callback)(OS_timestamp_t ts_now);

typedef void (*OS_sleep_wake_callback)(void);

typedef struct _OS_task_t
{
    OS_task_callback callback;
    OS_sleep_wake_callback enter_sleep;
    OS_sleep_wake_callback exit_sleep;
    void* p_next_task;
    OS_timestamp_t period;
    OS_timestamp_t prev_timestamp;
} OS_task_t;


/* ----------------------------------------------------------------------------
 * Public Function Declarations (Prototypes)
 * --------------------------------------------------------------------------*/
void OS_Start (void);
void OS_EnterLowPower (void);
void OS_ExitLowPower (void);
OS_timestamp_t OS_Get (void);
OS_timestamp_t OS_Elapsed (OS_timestamp_t ts);
void OS_LaunchDaemon (void);
bool OS_AddTask (OS_task_t* p_task);
bool OS_CreateTask (OS_task_t* p_task,
                    OS_timestamp_t period,
                    OS_task_callback callback,
                    OS_sleep_wake_callback sleep,
                    OS_sleep_wake_callback wake);


#endif //OS_API_H

