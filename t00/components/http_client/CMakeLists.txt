idf_component_register(SRCS "http_client.c" 
                    INCLUDE_DIRS "."
                    REQUIRES uart_console nvs_flash esp_http_client) 
