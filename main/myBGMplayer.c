#include "esp_log.h"
#include "io.h"
#include "player.h"
#include "sdcard.h"
#include "state.h"
#include <stdio.h> // For snprintf

static const char *TAG = "MY_BGM_PLAYER";

/**
 * @brief Helper to start playing the song currently selected in g_state
 */
static void play_current_song(void) {
  if (g_state.song_list.count == 0)
    return;

  // 1. Stop if playing
  mplayer_stop();

  // 2. Construct full path
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT,
           g_state.song_list.filenames[g_state.current_idx]);

  // 3. Play
  if (mplayer_play(filepath) == ESP_OK) {
    g_state.status = STATE_PLAYING;
    ESP_LOGI(TAG, "Playing: %s", filepath);
  } else {
    ESP_LOGE(TAG, "Failed to play: %s", filepath);
    g_state.status = STATE_STOPPED;
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "Starting BGM Player Initialization...");

  // 1. Setup IO Pins and Wakeup Sources
  io_setup_power_button();
  io_setup_buttons();

  // 2. Initialize Components
  if (sdcard_init() != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SD Card. System might be unstable.");
  }

  if (mplayer_setup() != ESP_OK) {
    ESP_LOGE(TAG, "Failed to setup Music Player.");
  }

  // 3. Initialize State (Scan for music)
  state_init(MOUNT_POINT);

  // 4. Create IO Management Tasks
  xTaskCreate(io_power_task, "power_task", 2048, NULL, 10, &power_task_handle);
  xTaskCreate(io_buttons_task, "buttons_task", 2048, NULL, 5,
              &buttons_task_handle);

  ESP_LOGI(TAG, "System Initialization Complete. Starting Main Loop...");

  // Auto-play first song if available
  if (g_state.song_list.count > 0) {
    play_current_song();
  }

  // 5. Main Loop
  while (1) {
    button_event_t btn = io_get_last_button();

    if (btn != BTN_NONE) {
      switch (btn) {
      case BTN_NEXT:
        ESP_LOGI(TAG, "CMD: Next Song");
        state_next_song();
        play_current_song();
        break;

      case BTN_PREV:
        ESP_LOGI(TAG, "CMD: Previous Song");
        state_prev_song();
        play_current_song();
        break;

      case BTN_PAUSE:
        ESP_LOGI(TAG, "CMD: Pause/Resume");
        if (g_state.status == STATE_PLAYING) {
          mplayer_pause();
          g_state.status = STATE_PAUSED;
        } else if (g_state.status == STATE_PAUSED) {
          mplayer_resume();
          g_state.status = STATE_PLAYING;
        } else if (g_state.status == STATE_STOPPED) {
          // Optional: If stopped, play current
          play_current_song();
        }
        break;

      default:
        break;
      }
    }

    // Optional: Check if song finished naturally (polling status or callback)
    // Since mplayer currently stops on EOF but doesn't auto-update status to
    // STOPPED in our structure, we might want to add a check here or update
    // mplayer to notify. For now, we rely on user input.
    if (mplayer_has_finished()) {
      state_next_song();
      play_current_song();
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Run loop at ~10Hz
  }
}
