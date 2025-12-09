#include "player.h"
#include "driver/dac_oneshot.h"
#include "driver/gptimer.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/dac_types.h" // For DAC_CHANNEL_1 if needed, usually in dac_oneshot.h
#include <stdio.h>
#include <string.h>

static const char *TAG = "PLAYER";

// Audio Configuration
#define SAMPLE_RATE 8000
#define TIMER_RESOLUTION_HZ 1000000 // 1MHz resolution
#define ALARM_COUNT (TIMER_RESOLUTION_HZ / SAMPLE_RATE)
#define BUFFER_SIZE 4096 // 4KB buffer

// State
static dac_oneshot_handle_t dac_handle = NULL;
static gptimer_handle_t timer_handle = NULL;
static TaskHandle_t player_task_handle = NULL;
static FILE *current_file = NULL;

// Buffer (Single Producer - Single Consumer Ring Buffer)
static uint8_t audio_buffer[BUFFER_SIZE];
static volatile size_t buf_head = 0; // Write index
static volatile size_t buf_tail = 0; // Read index
static volatile bool is_playing = false;
static volatile bool is_paused = false;

// Notification for the task
static SemaphoreHandle_t play_sem = NULL;

// Helper to check buffer fullness
static inline bool buffer_is_full(void) {
  return ((buf_head + 1) % BUFFER_SIZE) == buf_tail;
}

static inline bool buffer_is_empty(void) { return buf_head == buf_tail; }

static inline size_t buffer_free_space(void) {
  if (buf_head >= buf_tail) {
    return BUFFER_SIZE - 1 - (buf_head - buf_tail);
  } else {
    return buf_tail - buf_head - 1;
  }
}

// ISR - Executed at 8kHz
static bool IRAM_ATTR on_timer_alarm(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_ctx) {
  bool need_yield = false;

  if (is_playing && !is_paused) {
    if (!buffer_is_empty()) {
      uint8_t val = audio_buffer[buf_tail];
      buf_tail = (buf_tail + 1) % BUFFER_SIZE;

      // Output to DAC
      // Note: dac_oneshot_output_voltage is generally not guaranteed to be IRAM
      // safe or lock-free in all IDF versions, but for this prototype we assume
      // it works or overhead is acceptable. If strict real-time is needed,
      // direct register writing is preferred.
      dac_oneshot_output_voltage(dac_handle, val);
    } else {
      // Buffer Underflow - Output silence (mid-point or 0 depending on
      // encoding, assuming 8-bit unsigned PCM)
      // dac_oneshot_output_voltage(dac_handle, DAC_CHAN_1, 128);
    }
  }

  return need_yield;
}

// Player Task
static void player_task(void *arg) {
  uint8_t temp_chunk[256];
  size_t chunk_size = sizeof(temp_chunk);

  while (1) {
    // Wait for play signal
    if (!is_playing) {
      if (play_sem)
        xSemaphoreTake(play_sem, portMAX_DELAY);
    }

    if (is_playing && current_file) {
      if (is_paused) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }

      size_t free_space = buffer_free_space();
      if (free_space >= chunk_size) {
        size_t bytes_read = fread(temp_chunk, 1, chunk_size, current_file);

        if (bytes_read > 0) {
          for (size_t i = 0; i < bytes_read; i++) {
            audio_buffer[buf_head] = temp_chunk[i];
            buf_head = (buf_head + 1) % BUFFER_SIZE;
          }
        }

        if (bytes_read < chunk_size) {
          // EOF reached
          if (feof(current_file)) {
            ESP_LOGI(TAG, "End of file reached");
            // Wait for buffer to drain?
            // For simplicity, we just stop ensuring the last bits are played
            // Real implementation would wait for buffer_is_empty()
            while (!buffer_is_empty()) {
              vTaskDelay(pdMS_TO_TICKS(10));
            }

            is_playing = false;
            fclose(current_file);
            current_file = NULL;
            gptimer_stop(timer_handle);
          }
        }
      } else {
        // Buffer full, wait a bit
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    } else {
      // Should not happen if logic is correct
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

esp_err_t mplayer_setup(void) {
  ESP_LOGI(TAG, "Setting up Player...");

  // 1. DAC Setup
  dac_oneshot_config_t dac_cfg = {
      .chan_id = DAC_CHAN_1,
  };
  ESP_RETURN_ON_ERROR(dac_oneshot_new_channel(&dac_cfg, &dac_handle), TAG,
                      "Failed to create DAC channel");

  // 2. Timer Setup
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = TIMER_RESOLUTION_HZ,
  };
  ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &timer_handle), TAG,
                      "Failed to create timer");

  gptimer_event_callbacks_t cbs = {
      .on_alarm = on_timer_alarm,
  };
  ESP_RETURN_ON_ERROR(
      gptimer_register_event_callbacks(timer_handle, &cbs, NULL), TAG,
      "Failed to register callbacks");

  gptimer_alarm_config_t alarm_config = {
      .alarm_count = ALARM_COUNT,
      .reload_count = 0,
      .flags.auto_reload_on_alarm = true,
  };
  ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(timer_handle, &alarm_config),
                      TAG, "Failed to set alarm");
  ESP_RETURN_ON_ERROR(gptimer_enable(timer_handle), TAG,
                      "Failed to enable timer");

  // 3. Task Setup
  play_sem = xSemaphoreCreateBinary();
  BaseType_t ret = xTaskCreate(player_task, "player_task", 4096, NULL, 5,
                               &player_task_handle);
  if (ret != pdPASS) {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Player Setup Complete");
  return ESP_OK;
}

esp_err_t mplayer_play(char *filepath) {
  if (is_playing) {
    ESP_LOGW(TAG, "Already playing, stop first");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Opening file: %s", filepath);
  current_file = fopen(filepath, "rb");
  if (current_file == NULL) {
    ESP_LOGE(TAG, "Failed to open file");
    return ESP_FAIL;
  }

  // Reset buffer
  buf_head = 0;
  buf_tail = 0;
  is_paused = false;
  is_playing = true;

  // Start Timer
  ESP_ERROR_CHECK(gptimer_start(timer_handle));

  // Notify task
  xSemaphoreGive(play_sem);

  return ESP_OK;
}

esp_err_t mplayer_pause(void) {
  if (!is_playing)
    return ESP_FAIL;
  is_paused = true;
  ESP_LOGI(TAG, "Paused");
  return ESP_OK;
}

esp_err_t mplayer_resume(void) {
  if (!is_playing)
    return ESP_FAIL;
  is_paused = false;
  ESP_LOGI(TAG, "Resumed");
  return ESP_OK;
}

esp_err_t mplayer_stop(void) {
  ESP_LOGI(TAG, "Stopping Player...");

  // 1. Stop the timer (ISR)
  if (timer_handle) {
    gptimer_stop(timer_handle);
    gptimer_set_raw_count(timer_handle, 0);
  }

  // 2. Stop the logical playback
  is_playing = false;
  is_paused = false;

  // 3. Close file
  if (current_file) {
    fclose(current_file);
    current_file = NULL;
  }

  // 4. Clear Buffer
  memset(audio_buffer, 0, BUFFER_SIZE);
  buf_head = 0;
  buf_tail = 0;

  ESP_LOGI(TAG, "Player Stopped");
  return ESP_OK;
}
