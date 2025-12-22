#ifndef __FILES_H__
#define __FILES_H__

#include "esp_err.h"
#include <stddef.h> // For size_t

/**
 * @brief Structure to hold the list of files and their count.
 */
typedef struct {
    char **filenames; /**< Dynamically allocated array of filenames */
    size_t count;     /**< Number of files in the list */
} file_list_t;

/**
 * @brief Get a list of files in a given directory path.
 *
 * This function scans the specified directory and returns a dynamically allocated
 * list of filenames within that directory. The path should already include
 * the MOUNT_POINT (e.g., "/sdcard/music").
 *
 * @param dir_path The path to the directory to scan.
 * @param out_file_list Pointer to a file_list_t structure to store the results.
 *                      The 'filenames' member will be dynamically allocated.
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_NO_MEM if memory allocation fails.
 *      - ESP_ERR_NOT_FOUND if the directory does not exist or is not readable.
 *      - Other ESP_ERR_ codes if underlying file system operations fail.
 */
esp_err_t files_get_files_in_directory(const char *dir_path, file_list_t *out_file_list);

/**
 * @brief Free the memory allocated by files_get_files_in_directory.
 *
 * @param file_list Pointer to the file_list_t structure whose memory needs to be freed.
 */
void files_free_file_list(file_list_t *file_list);

#endif /* __FILES_H__ */