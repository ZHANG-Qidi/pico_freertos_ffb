/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif
#include "FreeRTOS.h"
#include "task.h"
// FFB
#include "adc_loop.h"
#include "adc_setup.h"
#include "ffb_loop.h"
#include "ffb_setup.h"
#include "foc_loop.h"
#include "foc_setup.h"
#include "tusb.h"
#include "usb_loop.h"
#include "usb_setup.h"

// Which core to run on if configNUMBER_OF_CORES==1
#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif
// Whether to flash the led
#ifndef USE_LED
#define USE_LED 1
#endif
// Delay between led blinking
#define LED_DELAY_MS 200
// Priorities of our threads - higher numbers are higher priority
#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + osPriorityBelowNormal)
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + osPriorityLow)
#define WORKER_TASK_PRIORITY (tskIDLE_PRIORITY + osPriorityNormal)
// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define BLINK_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define WORKER_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#include "pico/async_context_freertos.h"
static async_context_freertos_t async_context_instance;
// Create an async context
static async_context_t *create_async_context(void) {
    async_context_freertos_config_t config = async_context_freertos_default_config();
    config.task_priority = WORKER_TASK_PRIORITY;      // defaults to ASYNC_CONTEXT_DEFAULT_FREERTOS_TASK_PRIORITY
    config.task_stack_size = WORKER_TASK_STACK_SIZE;  // defaults to ASYNC_CONTEXT_DEFAULT_FREERTOS_TASK_STACK_SIZE
    if (!async_context_freertos_init(&async_context_instance, &config)) return NULL;
    return &async_context_instance.core;
}
#if USE_LED
// Turn led on or off
static void set_led(bool led_on) {
#if defined PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}
// Initialise led
static void init_led(void) {
#if defined PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    hard_assert(cyw43_arch_init() == PICO_OK);
    set_led(false);  // make sure cyw43 is started
#endif
}
void blink_task(__unused void *params) {
    bool on = false;
    printf("blink_task starts\n");
    init_led();
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(blink_interval_ms_usb_status));
        set_led(on);
        on = !on;
    }
}
#endif  // USE_LED
TaskHandle_t foc_task_handle = NULL;
bool repeating_timer_callback(struct repeating_timer *t) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(foc_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return true;
}
void foc_task(__unused void *params) {
    foc_setup();
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        foc_loop();
    }
}
void usb_task(__unused void *params) {
    usb_setup();
    for (;;) {
        // put this thread to waiting state until there is new events
        tud_task();
        // following code only run if tud_task() process at least 1 event
        usb_loop();
    }
}
void ffb_task(__unused void *params) {
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(USB_POLLING_INTERVAL));
    }
}
void adc_task(__unused void *params) {
    adc_setup();
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(ADC_READ_INTERVAL));
        adc_loop();
    }
}
// async workers run in their own thread when using async_context_freertos_t with priority WORKER_TASK_PRIORITY
static void do_work(async_context_t *context, async_at_time_worker_t *worker) { async_context_add_at_time_worker_in_ms(context, worker, 1); }
async_at_time_worker_t worker_timeout = {.do_work = do_work};
void main_task(__unused void *params) {
    async_context_t *context = create_async_context();
    // start the worker running
    async_context_add_at_time_worker_in_ms(context, &worker_timeout, 0);
#if USE_LED
    // start the led blinking
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "");
    xTaskCreate(blink_task, "BlinkThread", BLINK_TASK_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
#endif  // USE_LED
    xTaskCreate(foc_task, "foc_task", WORKER_TASK_STACK_SIZE, NULL, WORKER_TASK_PRIORITY, &foc_task_handle);
    xTaskCreate(usb_task, "usb_task", WORKER_TASK_STACK_SIZE, NULL, WORKER_TASK_PRIORITY, NULL);
    xTaskCreate(ffb_task, "ffb_task", WORKER_TASK_STACK_SIZE, NULL, WORKER_TASK_PRIORITY, NULL);
    xTaskCreate(adc_task, "adc_task", WORKER_TASK_STACK_SIZE, NULL, WORKER_TASK_PRIORITY, NULL);
    static struct repeating_timer timer;
    add_repeating_timer_us(-FOC_LOOP_PERIOD, repeating_timer_callback, NULL, &timer);
    TickType_t last = xTaskGetTickCount();
    while (true) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(LED_DELAY_MS));
    }
    async_context_deinit(context);
}
void vLaunch(void) {
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "");
    xTaskCreate(main_task, "MainThread", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}
int main(void) {
    stdio_init_all();
    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
    rtos_name = "FreeRTOS";
#if (RUN_FREE_RTOS_ON_CORE == 1 && configNUMBER_OF_CORES == 1)
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}
