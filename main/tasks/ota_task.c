#include "ota_task.h"
#include "rgb_state_task.h"
#include "intercom_constants.h"
#include "credentials.h"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_http_client.h"


const char *TAG_OTA = "intercom_ota";

static bool ota_post_diagnostic() {
    // Todo: write diagnostic checks
    bool diagnostic_is_ok = true;
    return diagnostic_is_ok;
}

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(){
    ESP_LOGE(TAG_OTA, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}



static void ota_via_http_client_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG_OTA, "Starting OTA example task");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG_OTA, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
                 configured->address, running->address);
        ESP_LOGW(TAG_OTA, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(TAG_OTA, "Running partition type %d subtype %d (offset 0x%08"PRIx32")",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = OTA_FIRMWARE_UPG_URL,
        .timeout_ms = OTA_FIRMWARE_RECV_TIMEOUT,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
        .disable_auto_redirect = false,
        .transport_type = HTTP_TRANSPORT_OVER_TCP,  // Force HTTP instead of HTTPS
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG_OTA, "Failed to initialise HTTP connection");
        task_fatal_error();
    }

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }

    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG_OTA, "Writing to partition subtype %d at offset 0x%"PRIx32,
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, OTA_BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG_OTA, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG_OTA, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG_OTA, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG_OTA, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG_OTA, "New version is the same as invalid version.");
                            ESP_LOGW(TAG_OTA, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG_OTA, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            task_fatal_error();
                        }
                    }
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG_OTA, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        task_fatal_error();
                    }
#endif

                    set_intercom_state(ENUM_INTERCOM_STATE_OTA_UPDATING);

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
                        ESP_LOGE(TAG_OTA, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG_OTA, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG_OTA, "received package is not fit len");
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG_OTA, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG_OTA, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG_OTA, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG_OTA, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG_OTA, "Error in receiving complete file");
        set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG_OTA, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG_OTA, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
        http_cleanup(client);
        return;
    }
    ESP_LOGI(TAG_OTA, "Prepare to restart system!");
    esp_restart();
}

void task_ota_start() {
    xTaskCreate(&ota_via_http_client_task, "ota_via_http_client_task", 8192, NULL, 5, NULL);
}   

void ota_check(){
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            bool diagnostic_is_ok = ota_post_diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG_OTA, "Diagnostics completed successfully! Continuing execution ...");
                set_intercom_state(ENUM_INTERCOM_STATE_OTA_SUCCESS);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG_OTA, "Diagnostics failed! Start rollback to the previous version ...");
                set_intercom_state(ENUM_INTERCOM_STATE_OTA_FAILURE);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}
