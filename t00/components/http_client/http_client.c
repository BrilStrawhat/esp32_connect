#include "http_client.h"

static const char *TAG = "HTTP_CLIENT";

static const char *REQUEST = "POST " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
// static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
    // switch(evt->event_id) {
        // case HTTP_EVENT_ERROR:
            // ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            // break;
        // case HTTP_EVENT_ON_CONNECTED:
            // ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            // break;
        // case HTTP_EVENT_HEADER_SENT:
            // ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            // break;
        // case HTTP_EVENT_ON_HEADER:
            // ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            // break;
        // case HTTP_EVENT_ON_DATA:
            // ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // if (!esp_http_client_is_chunked_response(evt->client)) {
                // // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            // }

            // break;
        // case HTTP_EVENT_ON_FINISH:
            // ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            // bteak;
        // case HTTP_EVENT_DISCONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            // // int mbedtls_err = 0;
            // // esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            // // if (err != 0) {
                // // ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                // // ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            // // }
            // break;
    // }
    // return ESP_OK;
// }

// void http_rest_post_url_ping(char *url, uint8_t repeat) {
    // esp_err_t err = 0;
    // const char *post_data = "{\"asd\": \"qwe\"}";
    // esp_http_client_config_t config = {
        // .url = url,
        // .method = HTTP_METHOD_POST,
        // .event_handler = _http_event_handler,
    // };
    // char str[32] = {0};

    // esp_http_client_handle_t client = esp_http_client_init(&config);

    // // esp_http_client_set_method(client, HTTP_METHOD_POST);
    // esp_http_client_set_post_field(client, post_data, strlen(post_data));
    // for(uint8_t i = 0; i < repeat; i++) {
        // err = esp_http_client_perform(client);
        // uart_print_nl();
        // uart_printstr("PING #");

        // if (err == ESP_OK) {
            // ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                    // esp_http_client_get_status_code(client),
                    // esp_http_client_get_content_length(client));
            // sprintf(str, "%d SUCCESS", i);
            // uart_printstr(str);
        // }
        // else {
            // ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
            // sprintf(str, "%d FAILED", i);
            // uart_printstr(str);
        // }
    // }
// }


