
#include "sdcard.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "sdmmc_cmd.h"

const char *tag = "SDCARD";

esp_err_t sdcard_init() {
  esp_err_t ret;
  sdspi_dev_handle_t hspidev;
  sdmmc_card_t card;

  ESP_LOGI(tag, "Initializing SDCard");

  // SPI Bus Initialization
  //
  // SDSPI Host Initialization
  ret = sdspi_host_init();
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed at Initializing the SDSPI host");
    return ret;
  }
  const sdspi_device_config_t spi_dev_config = SDSPI_DEVICE_CONFIG_DEFAULT();

  ret = sdspi_host_init_device(&spi_dev_config, &hspidev);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed at Initializing the SDSPI Host Device");
    return ret;
  }

  // initialize SDMMC Layer
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = hspidev;

  ret = sdmmc_card_init(&host, &card);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed at Initializing the SDMMC Card");
    return ret;
  }
  sdmmc_card_print_info(stdout, &card);

  return ESP_OK;
}
