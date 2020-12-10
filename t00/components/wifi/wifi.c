#include "wifi.h"

#define MAX_RECONNECT_TRY     1
#define WIFI_CREDENTIAL_DELIM ' '

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;
static uint8_t s_retry_num = 0;
static const char *TAG = "wifi station";

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RECONNECT_TRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        char str[16];
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(str, IPSTR, IP2STR(&event->ip_info.ip));
        uart_printstr("SUCCESS! Got IP:");
        uart_printstr(str);
        uart_print_nl();
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

char *strjoin_with_delim(char *s1, char *s2, char delim) {
    if (!s1 || !s2)
        return NULL;
    uint8_t len = strlen(s1) + strlen(s2) + 1;
    char *result = (char*)malloc(len * sizeof(char));
    uint8_t i = 0;

    memset(result, 0, len);
    for (i = 0; s1[i]; i++)
        result[i] = s1[i];
    result[i++] = delim;
    for (uint8_t k = 0; s2[k]; i++, k++)
        result[i] = s2[k];
    result[i] = '\0';
    return result;
}

void wifi_set_last_ssid(char *ssid, char *passwd) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    else {
        char *credentials = strjoin_with_delim(ssid, passwd, WIFI_CREDENTIAL_DELIM);

        err = nvs_set_str(nvs_handle, "last_ssid", credentials);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG, "Set credentials to nvs");
                break;
            default :
                ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
        }
        free(credentials);
    }
    nvs_close(nvs_handle);
}

void wifi_print_success_message(void) {
    wifi_ap_record_t ap_info;
    char str[32];

    esp_wifi_sta_get_ap_info(&ap_info);
    uart_printstr("sta_state");
    uart_print_nl();
    uart_printstr("State: CONNECTED");
    uart_print_nl();
    uart_printstr("SSID: ");
    uart_printstr((char*)ap_info.ssid);
    uart_print_nl();
    sprintf(str, "Channel: %d", ap_info.primary);
    uart_printstr(str);
    uart_print_nl();
    sprintf(str, "RRSI: %d dBm", ap_info.rssi);
    uart_printstr(str);
    uart_print_nl();
}

static void wifi_init_sta(char *ssid, char *passwd) {
    EventBits_t bits = 0;
    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
         .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, passwd);

    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();

        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_got_ip));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_sta finished.");
    }
    else {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                0);
        if (bits & WIFI_CONNECTED_BIT) {
            esp_wifi_disconnect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        esp_wifi_connect();
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    }

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        s_retry_num = 0;
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, passwd);
        wifi_set_last_ssid(ssid, passwd);
        wifi_print_success_message();
    }
    else if (bits & WIFI_FAIL_BIT) {
        s_retry_num = 0;
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, passwd);
        uart_print_nl();
        uart_printstr("Failed to connect to AP");
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // vEventGroupDelete(s_wifi_event_group);
}

uint8_t wifi_connect_nvs(void) {
    esp_err_t err;
    char credentials[97];
    nvs_handle_t nvs_handle;
    size_t length = 97;
    char *saveptr = NULL;
    char delim = ' ';
    char *ssid = NULL;
    char *passwd = NULL;

    err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    else {
        err = nvs_get_str(nvs_handle, "last_ssid", credentials, &length);
        switch (err) {
            case ESP_OK:
                ssid = strtok_r(credentials, &delim, &saveptr);
                passwd = strtok_r(NULL, &delim, &saveptr);
                if (ssid && passwd && !strtok_r(NULL, &delim, &saveptr))
                    wifi_init_sta(ssid, passwd);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG, "No last_ssid in nvs");
                break;
            default :
                ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
        }
    }
    nvs_close(nvs_handle);
    return 0;
}

void *wifi_sta_connect(void *arg) {
    if (arg == NULL || *(char*)arg == '\0') {
        return NULL;
    }
    else if (!strcmp(WIFI_STA_CONNECT_COMMAND, (char*)arg)) {
        uart_print_nl();
        uart_printstr("wifi_connect usage: ssid passwd");
        return NULL;
    }
    else if (!strcmp("kk", (char*)arg)) {
        wifi_init_sta("TP-Link_FF9C", "01431629");
        return NULL;
    }
    else {
        char *saveptr = NULL;
        char delim = ' ';
        char *ssid = strtok_r((char*)arg, &delim, &saveptr);
        char *passwd = strtok_r(NULL, &delim, &saveptr);

        if (ssid && passwd && !strtok_r(NULL, &delim, &saveptr))
            wifi_init_sta(ssid, passwd);
        else {
            uart_print_nl();
            uart_printstr("wifi_connect usage: ssid passwd");
        }
    }
    return NULL;
}
