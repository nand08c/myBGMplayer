#ifndef IO_H
#define IO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// GPIO Definitions
#define GPIO_POWER_BTN  39
#define GPIO_BTN_PREV   32
#define GPIO_BTN_PAUSE  33
#define GPIO_BTN_NEXT   35

// Button States
typedef enum {
    BTN_NONE = 0,
    BTN_PREV,
    BTN_PAUSE,
    BTN_NEXT
} button_event_t;

// Task Handles
extern TaskHandle_t power_task_handle;
extern TaskHandle_t buttons_task_handle;

/**
 * @brief Configures the ESP32 to wake up from deep sleep when GPIO_POWER_BTN is HIGH.
 *        Also configures the pin as input.
 */
void io_setup_power_button(void);

/**
 * @brief Task that monitors GPIO_POWER_BTN. 
 *        If it goes LOW, it puts the ESP32 into deep sleep.
 */
void io_power_task(void *pvParameters);

/**
 * @brief Configures the general purpose buttons (PREV, PAUSE, NEXT) as inputs.
 */
void io_setup_buttons(void);

/**
 * @brief Task that polls the button pins and updates the last button pressed variable.
 */
void io_buttons_task(void *pvParameters);

/**
 * @brief Returns the last button that was pressed.
 * @return The button event (BTN_PREV, BTN_PAUSE, BTN_NEXT, or BTN_NONE).
 */
button_event_t io_get_last_button(void);

#endif // IO_H
