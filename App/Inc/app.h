/**
 * @file    app.h
 * @brief   Main application state machine. Runs in the ThreadX application
 *          thread created in app_threadx.c.
 *
 *   INIT  -> mount SD, load config.json, start data logger
 *   RUN   -> every loop: safety service, heartbeat;
 *            every dev_interval: poll sensors + log to SD
 *   FAULT -> outputs off, report faults, wait until the cause is gone
 */
#ifndef APP_H
#define APP_H

typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_RUN,
    APP_STATE_FAULT
} app_state_t;

/** @brief Initialises the BSP pieces that need the kernel (semaphores). Call from App_ThreadX_Init(). */
void app_pre_kernel_init(void);

/** @brief Application thread body. Never returns. */
void app_run(void);

/** @brief Current state of the state machine. */
app_state_t app_get_state(void);

#endif /* APP_H */
