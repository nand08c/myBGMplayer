
idf_component_register(
  SRCS "src/sdcard.c" "src/player.c"
  INCLUDE_DIRS "include/"
  PRIV_REQUIRES vfs esp_driver_sdspi esp_driver_spi driver esp_driver_gpio fatfs esp_driver_gptimer esp_driver_dac
)
