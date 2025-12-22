#include "include/io.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "IO";

// Task Handles
TaskHandle_t power_task_handle = NULL;
TaskHandle_t buttons_task_handle = NULL;

// Variable to store the last button pressed
static volatile button_event_t last_button_pressed = BTN_NONE;

// --- Power Management ---

void io_setup_power_button(void) {
  // Configure power button GPIO
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << GPIO_POWER_BTN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE, // External pull-down/up assumed or
                                         // handled by switch logic
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  // Configure wakeup source: Wake up when pin is HIGH (1)
  // Note: gpio_wakeup_enable is for light sleep usually,
  // for deep sleep with ext0 we use esp_sleep_enable_ext0_wakeup
  esp_sleep_enable_ext0_wakeup(GPIO_POWER_BTN, 1);

  ESP_LOGI(TAG, "Power button setup complete. Wakeup on GPIO %d HIGH.",
           GPIO_POWER_BTN);
}

void io_power_task(void *pvParameters) {
  ESP_LOGI(TAG, "Power monitoring task started.");
  while (1) {
    // Check if the power switch is turned OFF (LOW)
    if (gpio_get_level(GPIO_POWER_BTN) == 0) {
      ESP_LOGI(TAG, "Power switch OFF detected. Entering Deep Sleep...");

      // Debounce/Safety delay to ensure it wasn't a glitch
      vTaskDelay(pdMS_TO_TICKS(100));
      if (gpio_get_level(GPIO_POWER_BTN) == 0) {
        esp_deep_sleep_start();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
  }
}

// --- General Buttons ---

void io_setup_buttons(void) {
  // Configure button GPIOs
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << GPIO_BTN_PREV) | (1ULL << GPIO_BTN_PAUSE) |
                      (1ULL << GPIO_BTN_NEXT),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en =
          GPIO_PULLUP_ENABLE, // Assume buttons connect to GND when pressed
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  ESP_LOGI(TAG, "Buttons setup complete.");
}

button_event_t io_get_last_button(void) {
  button_event_t temp = last_button_pressed;
  last_button_pressed =
      BTN_NONE; // Optional: clear after read if desired, or keep it.
                // User asked to "get the value", standard behavior for
                // "last pressed" usually implies consuming the event.
                // I will clear it to avoid repeated actions.
  return temp;
}

void io_buttons_task(void *pvParameters) {
  ESP_LOGI(TAG, "Button polling task started.");

  // State tracking for debouncing (simple state machine)
  int prev_state_prev = 1;
  int prev_state_pause = 1;
  int prev_state_next = 1;

  while (1) {
    int curr_prev = gpio_get_level(GPIO_BTN_PREV);
    int curr_pause = gpio_get_level(GPIO_BTN_PAUSE);
    int curr_next = gpio_get_level(GPIO_BTN_NEXT);

    // Logic for PREV Button (Active Low)
    if (prev_state_prev == 1 && curr_prev == 0) {
      last_button_pressed = BTN_PREV;
      ESP_LOGD(TAG, "Button PREV pressed");
    }

    // Logic for PAUSE Button (Active Low)
    if (prev_state_pause == 1 && curr_pause == 0) {
      last_button_pressed = BTN_PAUSE;
      ESP_LOGD(TAG, "Button PAUSE pressed");
    }

    // Logic for NEXT Button (Active Low)
    if (prev_state_next == 1 && curr_next == 0) {
      last_button_pressed = BTN_NEXT;
      ESP_LOGD(TAG, "Button NEXT pressed");
    }

    // Update states
    prev_state_prev = curr_prev;
    prev_state_pause = curr_pause;
    prev_state_next = curr_next;

    vTaskDelay(pdMS_TO_TICKS(50)); // Poll every 50ms
  }
}
