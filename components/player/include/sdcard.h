
#ifndef __SDCARD_H__
#define __SDCARD_H__

#include "esp_err.h"

#define MOUNT_POINT "/sdcard"

/**
 * Setup the sdcard using the sdmmc driver under the predefined pins:
 * CMD: 21
 * CLK: 22
 * D0: 23
 * D1: Not Used
 * D2: Not Used
 * D3: Not Used
 */
esp_err_t sdcard_init(void);

/**
 * Just Unmount and Try to cleanly end everything with her
 */
esp_err_t sdcard_detach(void);

#endif /* __SDCARD_H__ */
