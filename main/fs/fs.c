#include <stdio.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "FS";

void init_fs() {
    printf("Mounting SPIFFS...\n");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        printf("SPIFFS mount failed: %s\n", esp_err_to_name(ret));
        return;
    }

    printf("SPIFFS mounted successfully\n");

    size_t total = 0, used = 0;

    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        printf("SPIFFS info failed\n");
    } else {
        printf("SPIFFS: total=%d used=%d\n", total, used);
    }
}

void fs_list() {
    DIR *dir = opendir("/spiffs");
    struct dirent *entry;

    if (!dir) {
        printf("Failed to open dir\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}