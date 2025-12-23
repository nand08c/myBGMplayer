#include "fail.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" // For ets_delay_us

static const char *TAG = "FAIL";

// Define the Error LED Pin (Change this if using a different pin)
#define GPIO_ERROR_LED 2

void system_fatal_error(const char *msg) {
  // 1. Log the critical failure
  ESP_LOGE(TAG, "CRITICAL FAILURE: %s", msg);
  ESP_LOGE(TAG, "System halted. Blinking error LED on GPIO %d.",
           GPIO_ERROR_LED);

  // 2. Configure the LED Pin
  // We use direct register modification or simple GPIO calls.
  // Since we might be in a critical state, we keep it simple.
  gpio_reset_pin(GPIO_ERROR_LED);
  gpio_set_direction(GPIO_ERROR_LED, GPIO_MODE_OUTPUT);

  // 3. Infinite Loop Trap
  // We intentionally do NOT use vTaskDelay because the scheduler might be
  // compromised or we want to ensure we don't context switch.
  // However, to feed the WDT properly in a raw loop, we need to be careful.
  // The Task Watchdog (TWDT) protects tasks. The Interrupt Watchdog (IWDT)
  // protects ISRs.

  // If we are in a task, we can try to disable interrupts to lock everything,
  // but then we can't easily feed the TWDT without re-enabling them or handling
  // the hardware directly. A simpler approach for a "trap" that keeps the
  // device "alive" (blinking) is to stay in a loop and feed the WDT.

  while (1) {
    // Blink ON
    gpio_set_level(GPIO_ERROR_LED, 1);

    // Delay ~500ms using busy-wait (safer if FreeRTOS is broken)
    // ets_delay_us handles microseconds. 500ms = 500,000us
    for (int i = 0; i < 50; i++) {
      ets_delay_us(10000);  // 10ms chunks
      esp_task_wdt_reset(); // Feed the dog
    }

    // Blink OFF
    gpio_set_level(GPIO_ERROR_LED, 0);

    // Delay ~500ms
    for (int i = 0; i < 50; i++) {
      ets_delay_us(10000);  // 10ms chunks
      esp_task_wdt_reset(); // Feed the dog
    }
  }
}
