/******************************************************************************
 *  @file OS_api.c
 *
 *  This module contains the code to support the Operating System timer
 *  and foreground functionality.
 *
 */

/* ----------------------------------------------------------------------------
 * Dependencies
 * --------------------------------------------------------------------------*/
#include "cytypes.h"
#include "cyfitter.h"
#include <stddef.h>
#include "OS_core_api.h"
#include "OS_Wdt0Irq.h"
#include "CyLib.h"
#include "cyPm.h"


/* ----------------------------------------------------------------------------
 * Private Constant and Macro Definitions
 * --------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------
 * Private Type Definitions
 * --------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------
 * Private Data Declarations
 * --------------------------------------------------------------------------*/
/** Local pointer to the first task in the list of OS-managed tasks */
static OS_task_t* p_first_task_config = NULL;
/** Local pointer to the last task in the list of OS-managed tasks */
static OS_task_t* p_last_task_config = NULL;

/** Local Boolean indicating whether or not the OS is enabled */
static bool is_os_active = false;
/** Local Boolean indicating whether or not the OS sleep state is enabled */
static bool is_sleep_active = false;

static OS_timestamp_t isr_counter = 0;
static OS_timestamp_t ms_counter = 0;
static bool mutex = false;


/* ----------------------------------------------------------------------------
 * Private Function Declarations (Prototypes)
 * --------------------------------------------------------------------------*/
CY_ISR(OS_Wdt0Isr);


/* ----------------------------------------------------------------------------
 * Public Function Definitions
 * --------------------------------------------------------------------------*/
/**
 *  This public functiton sets up the states and resources for the OS component.
 *
 *  This function configures the Watchdog Timer 0 to interrupt
 *  periodically, triggering the WDT ISR defined in this module.
 *  This is accomplished by setting the mode of the WDT0 to interrupt on match,
 *  setting it to clear its counter when the match occurs, and setting the
 *  match value to the PRESCALE_AWAKE component parameter. The WDT0 is enabled,
 *  and the ISR component (which is tied to the WDT's interrupt output) is
 *  vectored to the ISR function above its priority is set to the highest to
 *  ensure that it occurs on time.
 *
 *  This function initializes the static is_os_active Boolean to true,
 *  indicating that the OS has been initialized and started.
 *
 *  This function initializes the static is_sleep_active Boolean to false,
 *  indicating that the Sleep mode of the OS is not activated.
 *
 *  This function should be called before the Global Interrupts are enabled.
 */
void OS_Start (void)
{
    CySysWdtWriteMode(CY_SYS_WDT_COUNTER0, CY_SYS_WDT_MODE_INT);
    CySysWdtWriteMatch(CY_SYS_WDT_COUNTER0, 32);
    CySysWdtWriteClearOnMatch(CY_SYS_WDT_COUNTER0, 1);

    CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);
    while(!CySysWdtReadEnabledStatus(CY_SYS_WDT_COUNTER0));

    OS_Wdt0Irq_StartEx(OS_Wdt0Isr);
    OS_Wdt0Irq_SetPriority(0);

    is_os_active = true;
    is_sleep_active = false;
}


/**
 *  This public function sets the states of the OS such that it stops running.
 *
 *  This function sets the is_os_active Boolean to false.
 */
void OS_Stop (void)
{
    is_os_active = false;
}


/**
 *  This public function performs the transition into the Sleep mode of the OS.
 *
 *  This function first checks to make sure that Sleep mode is not already
 *  activated by checking that the is_sleep_active Boolean is not already
 *  true before performing any other actions.
 *
 *  This function loops through the linked list of tasks that have been
 *  instantiated and added to the OS and calls the enter_sleep callback of
 *  each task that has one defined. This allows each task to prepare itself
 *  to operate the way that it needs to while Sleep mode is active.
 *
 *  This function sets the is_sleep_active Boolean to true.
 */
void OS_EnterLowPower (void)
{
    uint8 int_state;
    OS_task_t* p_active_task;

    if (!is_sleep_active)
    {
        int_state = CyEnterCriticalSection();
        CySysWdtDisable(CY_SYS_WDT_COUNTER0_MASK);
        CyDelay(10);
        CySysWdtWriteMatch(CY_SYS_WDT_COUNTER0, 32);
        CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);
        p_active_task = p_first_task_config;
        while (NULL != p_active_task)
        {
            if (NULL != p_active_task->enter_sleep)
            {
                p_active_task->enter_sleep();
            }
            p_active_task = p_active_task->p_next_task;
        }
        CyExitCriticalSection(int_state);

        is_sleep_active = true;
    }
}


/**
 *  This public function performs the transition out of the Sleep mode of the OS.
 *
 *  This function first checks to make sure that Sleep mode activated by
 *  checking that the is_sleep_active Boolean is true before performing any
 *  other actions.
 *
 *  This function loops through the linked list of tasks that have been
 *  instantiated and added to the OS and calls the exit_sleep callback of
 *  each task that has one defined. This allows each task to return to
 *  its normal, non-Sleep-mode operation state.
 *
 *  This function sets the is_sleep_active Boolean to false.
 */
void OS_ExitLowPower (void)
{
    uint8 int_state;
    OS_task_t* p_active_task;

    if (is_sleep_active)
    {
        int_state = CyEnterCriticalSection();
        CySysWdtDisable(CY_SYS_WDT_COUNTER0_MASK);
        CyDelay(10);
        CySysWdtWriteMatch(CY_SYS_WDT_COUNTER0, 32);
        CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);
        is_sleep_active = false;
        p_active_task = p_first_task_config;
        while (NULL != p_active_task)
        {
            if (NULL != p_active_task->exit_sleep)
            {
                p_active_task->exit_sleep();
            }
            p_active_task = p_active_task->p_next_task;
        }
        CyExitCriticalSection(int_state);
    }
}


/**
 *  This public function returns the present value of the system millisecond
 *  counter.
 *
 *  This function sets the static mutex flag to true before reading the counter
 *  value and returns it to false after reading the counter value.
 *
 *  This function returns the value read from the mutex-locked system
 *  millisecond counter static variable.
 *
 *  @return Present system millisecond counter value
 */
OS_timestamp_t OS_Get (void)
{
    OS_timestamp_t retval;
    mutex = true;
    retval = ms_counter;
    mutex = false;
    return retval;
}


/**
 *  This public function returns the number of milliseconds that has elapsed
 *  from the passed timestamp until now.
 *
 *  This function calls the Get method of this component to read the present
 *  value of the system millisecond counter and returns the difference between
 *  that value and the passed timestamp. This will reflect the number of
 *  milliseconds that has elapsed since that timestamp was created.
 *
 *  The limitation of this function is that elapsed times of greater than the
 *  size of the timestamp type will alias to modulus of that time.
 *
 *  @param ts The millisecond timestamp of the past event to time to
 *  @return Elapsed milliseconds from the passed timestamp until now
 */
OS_timestamp_t OS_Elapsed (OS_timestamp_t ts)
{
    return (OS_Get() - ts);
}


/**
 *  This public blocking function runs the OS until it is stopped.
 *
 *  This function sets the is_os_active Boolean to true to ensure that the
 *  OS is activated when this function starts. It then enters an indefinite
 *  loop that will continue until that is_os_active Boolean is switched to
 *  false--stopping the OS from running and returning to the calling function.
 *
 *  This function calls the Get method of this component to read the present
 *  value of the system millisecond counter and uses that value for all time-
 *  related activities during this loop iteration, thus minimizing any unintended
 *  consequences of the time updating during the iteration.
 *
 *  This function loops through the linked list of tasks that have been
 *  instantiated and added to the OS and calls the callback of each task that
 *  for which the time since its last execution (stored in its prev_timestamp)
 *  is greater than or equal to its defined period. If a task's callback is
 *  called, then its prev_timestamp is updated to the present timestamp.
 *
 *  This function calls CySysPmDeepSleep to enter the sleep mode until the
 *  next WDT0 tick interrupt if the is_sleep_active Boolean is true, otherwise
 *  it immediately proceeds to the next loop iteration.
 */
void OS_LaunchDaemon (void)
{
    OS_timestamp_t now;
    OS_task_t* p_active_task;

    is_os_active = true;
    while (is_os_active)
    {
        now = OS_Get();

        p_active_task = p_first_task_config;
        while (NULL != p_active_task)
        {
            /**** Note: This cast is necessary to get the proper truncation of unsigned subtractions ****/
            if ((OS_timestamp_t)(now - p_active_task->prev_timestamp) >= p_active_task->period)
            {
                p_active_task->callback(now);
                p_active_task->prev_timestamp = now;
            }
            p_active_task = p_active_task->p_next_task;
        }

        if (is_sleep_active)
        {
            #if 0
            uint32_t temp_reg = CY_GET_REG32(CYREG_GPIO_PRT3_PC);
            CY_SET_REG32(CYREG_GPIO_PRT3_PC, temp_reg & 0xFF00003F);
            #endif

            CySysPmDeepSleep();

            #if 0
            CY_SET_REG32(CYREG_GPIO_PRT3_PC, temp_reg);
            #endif
        }
    }
}


/**
 *  This public function adds the passed task to the linked list of tasks for
 *  the OS to manage.
 *
 *  This function checks to see if there is task in the list. If not, then the
 *  pointer to the passed tasks is added to the start of the queue. If so, then
 *  the pointer to the passed task is added to the next_task pointer of the
 *  last task in the list. In either case, the passed task is set at the last
 *  task in the list and its next_task pointer is set to NULL.
 *
 *  @param p_task Pointer to a static instance of a task object
 *  @return Always true
*/
bool OS_AddTask (OS_task_t* p_task)
{
    //
    // If there is not already a task, then the passed one is the first one.
    //
    if (NULL == p_first_task_config)
    {
        p_first_task_config = p_task;
    }
    //
    // If there is already at least one task, then point the link from the last one to the new one.
    //
    else
    {
        p_last_task_config->p_next_task = p_task;
    }
    //
    // In either case, the one passed in is the last one in the list, and doesn't point to another.
    //
    p_last_task_config = p_task;
    p_last_task_config->p_next_task = NULL;
    return true;
}


/**
 *  This public function populates the passed task and adds it to the linked
 *  list of tasks for the OS to manage.
 *
 *  This function loads the passed task instance with the passed parameters,
 *  the present timestamp, and NULL next_task pointer then calls the AddTask
 *  method to add it to the OS task list.
 *
 *  @param p_task Pointer to a static instance of a task object
 *  @param period Time in milliseconds between executions of the task
 *  @param callback The function to execute when the task is ready to run
 *  @param sleep The function to execute to prepare the task to enter sleep
 *  @param wake The function to execute to prepare the task to exit sleep
 *  @return The value returned by the call to the AddTask method
*/
bool OS_CreateTask (OS_task_t* p_task,
                                  OS_timestamp_t period,
                                  OS_task_callback callback,
                                  OS_sleep_wake_callback sleep,
                                  OS_sleep_wake_callback wake)
{
    p_task->period = period;
    p_task->callback = callback;
    p_task->enter_sleep = sleep;
    p_task->exit_sleep = wake;
    p_task->p_next_task = NULL;
    p_task->prev_timestamp = OS_Get();
    return (OS_AddTask(p_task));
}



/* ----------------------------------------------------------------------------
 * ISR Definitions
 * --------------------------------------------------------------------------*/
/**
 *  This ISR function performs the periodic tick updates based on the WDT0
 *  settings.
 *
 *  This function clears the interrupt source in the WDT0 component.
 *
 *  This function increments a static isr_counter each time it occurs. If the
 *  mutex has not been claimed (is false), then the static system millisecond
 *  counter is increased by the value of this isr_counter, and the isr_counter
 *  is cleared. If the mutex is claimed, then the occurence of this tick is
 *  not lost because of the isr_counter which will retain this count until a
 *  non-locked occurence of the ISR runs.
 *
 *  If the processor was asleep, this ISR runs and then returns from the
 *  CySysPmDeepSleep call running in active (awake).
 */
CY_ISR(OS_Wdt0Isr)
{
    CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
    isr_counter++;
    if (!mutex)
    {
        ms_counter += isr_counter;
        isr_counter = 0;
    }
}


/* ---------------------------------------------------------------------------
 * Private Function Definitions
 * -------------------------------------------------------------------------*/

