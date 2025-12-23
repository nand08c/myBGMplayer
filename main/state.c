#include "state.h"
#include "esp_log.h"
#include "fail.h"
#include <string.h>

static const char *TAG = "STATE";

// Global instance of the player state
player_state_t g_state = {
    .current_idx = 0,
    .status = STATE_STOPPED
};

void state_init(const char *dir_path) {
    ESP_LOGI(TAG, "Initializing state from directory: %s", dir_path);
    
    esp_err_t ret = files_get_files_in_directory(dir_path, &g_state.song_list);
    if (ret != ESP_OK) {
        system_fatal_error("Failed to read music directory from SD Card");
    }

    if (g_state.song_list.count == 0) {
        ESP_LOGW(TAG, "No songs found in %s", dir_path);
    } else {
        ESP_LOGI(TAG, "State initialized with %zu songs.", g_state.song_list.count);
    }
    
    g_state.current_idx = 0;
    g_state.status = STATE_STOPPED;
}

void state_next_song(void) {
    if (g_state.song_list.count == 0) return;
    
    g_state.current_idx = (g_state.current_idx + 1) % g_state.song_list.count;
    ESP_LOGI(TAG, "Switched to next song, index: %d", g_state.current_idx);
}

void state_prev_song(void) {
    if (g_state.song_list.count == 0) return;
    
    g_state.current_idx--;
    if (g_state.current_idx < 0) {
        g_state.current_idx = g_state.song_list.count - 1;
    }
    ESP_LOGI(TAG, "Switched to previous song, index: %d", g_state.current_idx);
}
