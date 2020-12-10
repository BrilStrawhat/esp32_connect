#include "wifi.h"
#include "uart_console.h"
#include "nvs_flash.h"
#include "http_client.h"

void *http(void *arg) {
    if (arg == NULL || *(char*)arg == '\0')
        return NULL;
    char *saveptr = NULL;
    char delim = ' ';
    char *method = strtok_r((char*)arg, &delim, &saveptr);

    if (!strcmp(method, "kk")) {
        http_rest_post_url_ping("http://192.168.0.102:5000", 3);
        return NULL;
    }
    if (method && !strcmp(method, "post")) {
        char *cmd = strtok_r(NULL, &delim, &saveptr);

        if (cmd && !strcmp(cmd, "echo")) {
            char *url = strtok_r(NULL, &delim, &saveptr); 
            char *port = strtok_r(NULL, &delim, &saveptr); 
            char *repeat = strtok_r(NULL, &delim, &saveptr); 

            if (url && port && repeat) {
                uint16_t len = strlen(url) + strlen(url); 
                char *url_port = (char*)malloc(len * sizeof(char)) + 8;

                sprintf(url_port, "http://%s:%s", url, port);
                http_rest_post_url_ping(url_port, atoi(repeat));
                free(url_port);
            }
        }

    }
    else {
        uart_print_nl();
        uart_printstr("http usage: http method command args");
    }
    // http_rest_with_url();
    return NULL;
}



void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    add_cmd(WIFI_STA_CONNECT_COMMAND, wifi_sta_connect);
    add_cmd("http", http);
    init_uart_console(&uart_config, 1);
    wifi_connect_nvs();
}
