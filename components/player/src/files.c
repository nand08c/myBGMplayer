#include "files.h"
#include "esp_log.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "FILES";

esp_err_t files_get_files_in_directory(const char *dir_path,
                                       file_list_t *out_file_list) {
  DIR *dp = NULL;
  struct dirent *entry = NULL;
  char **filenames = NULL;
  size_t count = 0;
  esp_err_t ret = ESP_OK;

  // Initialize out_file_list
  out_file_list->filenames = NULL;
  out_file_list->count = 0;

  ESP_LOGI(TAG, "Scanning directory: %s", dir_path);

  // Check if directory exists and is accessible
  struct stat st;
  if (stat(dir_path, &st) == -1) {
    ESP_LOGE(TAG, "Failed to stat directory %s", dir_path);
    return ESP_ERR_NOT_FOUND;
  }
  if (!S_ISDIR(st.st_mode)) {
    ESP_LOGE(TAG, "%s is not a directory", dir_path);
    return ESP_ERR_NOT_FOUND;
  }

  dp = opendir(dir_path);
  if (dp == NULL) {
    ESP_LOGE(TAG, "Failed to open directory %s", dir_path);
    return ESP_ERR_NOT_FOUND;
  }

  // First pass: count files
  while ((entry = readdir(dp)) != NULL) {
    // Ignore . and ..
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    // Only add regular files, not subdirectories
    // Note: d_type can be DT_UNKNOWN on some file systems, requiring stat()
    // For simplicity, we assume d_type is reliable or we only care about names
    // For actual files, we'd typically want to append dir_path and then stat
    // But for listing names, this is sufficient.
    // if (entry->d_type == DT_REG) { // DT_REG = regular file
    count++;
    // }
  }

  if (count == 0) {
    ESP_LOGW(TAG, "No files found in directory %s", dir_path);
    closedir(dp);
    return ESP_OK; // No files, but not an error
  }

  // Allocate memory for filenames array
  filenames = (char **)calloc(count, sizeof(char *));
  if (filenames == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for filenames array");
    closedir(dp);
    return ESP_ERR_NO_MEM;
  }

  // Rewind directory stream for second pass
  rewinddir(dp);
  size_t current_idx = 0;

  // Second pass: store filenames
  while ((entry = readdir(dp)) != NULL && current_idx < count) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    // if (entry->d_type == DT_REG) {
    filenames[current_idx] = strdup(entry->d_name);
    if (filenames[current_idx] == NULL) {
      ESP_LOGE(TAG, "Failed to allocate memory for filename %s", entry->d_name);
      ret = ESP_ERR_NO_MEM;
      break; // Exit loop and clean up
    }
    current_idx++;
    // }
  }

  closedir(dp);

  if (ret != ESP_OK) {
    // Cleanup if an error occurred during second pass
    for (size_t i = 0; i < current_idx; i++) {
      free(filenames[i]);
    }
    free(filenames);
  } else {
    out_file_list->filenames = filenames;
    out_file_list->count = count;
    ESP_LOGI(TAG, "Found %zu files in %s", count, dir_path);
  }

  return ret;
}

void files_free_file_list(file_list_t *file_list) {
  if (file_list && file_list->filenames) {
    for (size_t i = 0; i < file_list->count; i++) {
      free(file_list->filenames[i]);
    }
    free(file_list->filenames);
    file_list->filenames = NULL;
    file_list->count = 0;
  }
}
