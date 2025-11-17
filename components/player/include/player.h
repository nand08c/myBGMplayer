

#ifndef __PLAYER_H__
#define __PLAYER_H__

/**
 * This the music player component of the music player, it works by first
 * opening a file under a FS somewhere, the file format isn't really decided
 * still, so for now let's just assume that the file can be read, ignoring if
 * the file has any compression applied then the file is read continuosly into a
 * buffer inside a task a previous setted interrupt will take data from that
 * buffer and pass it to the DAC1 in the ESP32, when the buffer is used to a
 * certain value it refills so the music can work smothly
 */

#include "esp_err.h"

/**
 * Setup the Task and Interrupt for the music player inner workings
 */
esp_err_t mplayer_setup(void);

/**
 * Play a song residing in the file indicated
 */
esp_err_t mplayer_play(char *filepath);

/**
 * Function to pause and resume the song, work by toggling the interrupt on/off
 */
esp_err_t mplayer_pause(void);
esp_err_t mplayer_resume(void);

#endif /* __PLAYER_H__ */
