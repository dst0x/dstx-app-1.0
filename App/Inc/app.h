/*
 * app.h
 *
 *  Created on: Sep 27, 2026
 *      Author: DSTX Labs
 */

#ifndef APP_H
#define APP_H

typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_RUN,
    APP_STATE_FAULT
} app_state_t;

void app_pre_kernel_init(void);
void app_run(void);

app_state_t app_get_state(void);

#endif /* APP_H */
