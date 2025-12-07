
#include "sdcard.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "hal/spi_types.h"
#include "sdmmc_cmd.h"
#include "soc/soc.h"

#define MOUNT_POINT "/sdcard"

const char *tag = "SDCARD";

sdmmc_card_t *card;
esp_err_t sdcard_init() {
  esp_err_t ret;

  ESP_LOGI(tag, "Initializing SDCard");

  // SPI Bus Initialization
  const spi_bus_config_t bus_cfg = {.max_transfer_sz = 0,
                                    .mosi_io_num = 21,
                                    .sclk_io_num = 22,
                                    .miso_io_num = 23,
                                    .quadhd_io_num = -1,
                                    .quadwp_io_num = -1,
                                    .data4_io_num = -1,
                                    .data5_io_num = -1,
                                    .data6_io_num = -1,
                                    .data7_io_num = -1,
                                    .isr_cpu_id = PRO_CPU_NUM,
                                    .flags = 0};

  ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed to initialize spi bus");
    return ret;
  }

  // SDSPI Host Initialization
  sdspi_device_config_t spi_dev_config = SDSPI_DEVICE_CONFIG_DEFAULT();

  // initialize SDMMC Layer
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;

  const esp_vfs_fat_mount_config_t mount_cfg = {.format_if_mount_failed = false,
                                                .max_files = 5,
                                                .allocation_unit_size =
                                                    16 * 1024};

  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &spi_dev_config,
                                          &mount_cfg, &card);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed to mount filesystem! Error: %s", esp_err_to_name(ret));
    return ret;
  }

  sdmmc_card_print_info(stdout, card);

  return ESP_OK;
}

esp_err_t sdcard_detach() {
  ESP_LOGI(tag, "Detaching SDCard");
  esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed to unmount SDCard");
    return ret;
  }

  ret = spi_bus_free(SPI2_HOST);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "Failed to free SPI bus");
    return ret;
  }

  ESP_LOGI(tag, "SDCard Detached");
  return ESP_OK;
}
