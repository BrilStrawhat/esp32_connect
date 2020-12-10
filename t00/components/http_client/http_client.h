#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "192.168.0.102"
#define WEB_PORT "5000"
#define WEB_PATH "/"

#include "uart_console.h"

void http_rest_post_url_ping(char *url, uint8_t repeat);
#endif 
