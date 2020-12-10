#ifndef _UART_CONSOLE_H_
#define _UART_CONSOLE_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "list.h"

#define UART_NUM UART_NUM_1
#define UART_TX     17
#define UART_RX     16

#define BUF_SIZE    256
#define CMD_SIZE    255

#define LEFT_ARR    4479771 // if read uart response as 4 byte int
#define RIGHT_ARR   4414235 // if read uart response as 4 byte int

#define DEL_SEQ     "\e[P"
#define LEFT_SEQ    "\e[D"
#define RIGHT_SEQ   "\e[C"
#define NL_SEQ      "\eE"

typedef struct s_console_cmd {
    void *(*cmd_handler)(void *argc);
    char *cmd_name;
} t_console_cmd;

void init_uart_console(uart_config_t *uart_config, uint8_t console_priority);

void clear_cmd_list();

void add_cmd(char *cmd_name, void *(*cmd_handler)(void *argc));

void uart_printstr(char *str);
void uart_print_nl();
#endif 
