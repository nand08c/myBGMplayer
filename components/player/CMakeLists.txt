
idf_component_register(
  SRCS "src/sdcard.c"
  INCLUDE_DIRS "include/"
  PRIV_REQUIRES vfs esp_driver_sdspi esp_driver_spi driver esp_driver_gpio fatfs
)
