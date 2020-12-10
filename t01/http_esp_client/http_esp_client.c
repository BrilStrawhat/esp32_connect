#include "http_esp_client.h"

static const char *TAG = "HTTP_CLIENT";

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            bteak;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            // int mbedtls_err = 0;
            // esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            // if (err != 0) {
                // ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                // ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            // }
            break;
    }
    return ESP_OK;
}

void http_rest_post_url_ping(char *url, uint8_t repeat) {
    esp_err_t err = 0;
    const char *post_data = "{\"asd\": \"qwe\"}";
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };
    char str[32] = {0};

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    for(uint8_t i = 0; i < repeat; i++) {
        err = esp_http_client_perform(client);
        uart_print_nl();
        uart_printstr("PING #");

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
            sprintf(str, "%d SUCCESS", i);
            uart_printstr(str);
        }
        else {
            ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
            sprintf(str, "%d FAILED", i);
            uart_printstr(str);
        }
    }
}
