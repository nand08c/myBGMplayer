#include "esp_log.h"
#include "player.h"
#include "sdcard.h"

void app_main(void) {
  ESP_LOGI("MY_BGM_PLAYER", "Hello World");
  sdcard_init();
  mplayer_setup();
  mplayer_play(MOUNT_POINT "/wir8am.wav");
}
