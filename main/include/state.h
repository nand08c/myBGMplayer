#ifndef STATE_H
#define STATE_H

#include "files.h"
#include <stdbool.h>

/**
 * @brief Enumeration for the current playback status.
 */
typedef enum {
    STATE_STOPPED = 0,
    STATE_PLAYING,
    STATE_PAUSED
} player_status_t;

/**
 * @brief Structure to hold the complete state of the music player.
 */
typedef struct {
    file_list_t song_list;      /**< List of songs found on the SD card */
    int current_idx;            /**< Index of the song currently playing/selected */
    player_status_t status;     /**< Current playback status */
} player_state_t;

/**
 * @brief Global player state instance.
 */
extern player_state_t g_state;

/**
 * @brief Initializes the player state by scanning the SD card for files.
 * @param dir_path The directory to scan for music files.
 */
void state_init(const char *dir_path);

/**
 * @brief Moves the state to the next song in the list.
 */
void state_next_song(void);

/**
 * @brief Moves the state to the previous song in the list.
 */
void state_prev_song(void);

#endif // STATE_H
