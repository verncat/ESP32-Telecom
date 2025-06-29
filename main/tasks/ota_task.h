#include "stdbool.h"

#define OTA_BUFFSIZE 1024
static char ota_write_data[OTA_BUFFSIZE + 1] = { 0 };

static bool ota_post_diagnostic();

void ota_task(void *pvParameter);

void ota_check();

void task_ota_start();