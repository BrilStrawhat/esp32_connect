#include "uart_console.h"

static const char *TAG = "uart_events";
static QueueHandle_t uart0_queue;
static t_list *cmd_list = NULL;

static void move_arrow(uint8_t *cursor, char *command, uint32_t dtmp) {
    if (dtmp == LEFT_ARR) {
        if (*cursor > 0) {
            uart_write_bytes(UART_NUM, LEFT_SEQ, 3);
            (*cursor)--;
            ESP_LOGI(TAG, "left move, cur: %d", *cursor);
        }
    }
    else if (dtmp == RIGHT_ARR) {
        if (*cursor < CMD_SIZE && command[*cursor] != '\0') {
            uart_write_bytes(UART_NUM, RIGHT_SEQ, 3);
            (*cursor)++;
            ESP_LOGI(TAG, "right move, cur: %d", *cursor);
        }
    }
}

static void backspace(uint8_t *cursor, char *command) {
    uint8_t size_of_left_piece = strlen(command + *cursor);

    ESP_LOGI(TAG, "backspace");
    ESP_LOGI(TAG, "shift: %s, %d", command + *cursor, size_of_left_piece);
    if (*cursor == 0)
        return;
    if (size_of_left_piece == 0)
        command[*cursor - 1] = '\0';
    else {
        memmove(command + *cursor - 1, command + *cursor, size_of_left_piece);
        command[*cursor + size_of_left_piece - 1] = '\0';
    }
    (*cursor)--;
    uart_write_bytes(UART_NUM, LEFT_SEQ, 3);
    uart_write_bytes(UART_NUM, DEL_SEQ, 3);
    ESP_LOGI(TAG, "after shift: %s, %d", command, *cursor + size_of_left_piece -1);
}

static void put_char_under_cursor(uint8_t *cursor, char *command, char ch) {
    char insert_char[] = "\e[@\0";
    uint8_t size_of_left_piece = strlen(command + *cursor);

    if (*cursor + size_of_left_piece >= CMD_SIZE)
        return;
    ESP_LOGI(TAG, "put_char_under_cursor");
    ESP_LOGI(TAG, "shift: %s, %d", command + *cursor, size_of_left_piece);
    memmove(command + *cursor + 1, command + *cursor, size_of_left_piece);
    command[*cursor] = ch;
    (*cursor)++;
    ESP_LOGI(TAG, "after shift: %s, cur:%d", command, *cursor);
    insert_char[3] = ch;
    uart_write_bytes(UART_NUM, insert_char, 4);
}

static void printchar(uint8_t *cursor, char *command, char *dtmp, uart_event_t *event) {
    if (*cursor == CMD_SIZE)
        return;
    if (command[*cursor] == '\0') {
        command[*cursor] = *dtmp;
        uart_write_bytes(UART_NUM, (const char*)dtmp, event->size);
        (*cursor)++;
        ESP_LOGI(TAG, "[DATA EVT]: %s, %d, cur = %d",
                dtmp, *dtmp, *cursor);
    }
    else
        put_char_under_cursor(cursor, command, *dtmp);
}

bool exec_cmd_by_name(t_list *cmd_list, char *name, char *args) {
    while (cmd_list) {
        if (((t_console_cmd*)cmd_list->data)->cmd_name && name) {
            if (strcmp(((t_console_cmd*)cmd_list->data)->cmd_name, name) == 0) {
                ((t_console_cmd*)cmd_list->data)->cmd_handler(args);
                return true;
            }
        }
        cmd_list = cmd_list->next;
    }
    if (*name != '\0') {
        uart_print_nl();
        uart_printstr("Command '");
        uart_printstr(name);
        uart_printstr("' not found");
    }
    return false;
}

static void enter_button(char *command) {
    char *space = strchr(command, ' ');

    if (space != NULL) {
        uint8_t space_index = (uint8_t)(space - command);
        char *cmd_name = (char*)malloc(space_index);

        memcpy(cmd_name, command, space_index);
        *(cmd_name + space_index) = '\0';
        ESP_LOGI(TAG, "[CMD_NAME]: %s", cmd_name);
        ESP_LOGI(TAG, "[COMMAND]: %s", command);
        exec_cmd_by_name(cmd_list, cmd_name, command + space_index + 1);
        free(cmd_name);
    }
    else {
        ESP_LOGI(TAG, "[COMMAND]: %s", command);
        exec_cmd_by_name(cmd_list, command, command);
    }
    uart_write_bytes(UART_NUM, NL_SEQ, 2);
    memset(command, 0, CMD_SIZE);
}

static void console_task(void *pvParameters) {
    uart_event_t event;
    uint8_t cursor = 0;
    char *command = (char *)malloc(sizeof(char) * CMD_SIZE + 1);
    char *dtmp = (char *)malloc(sizeof(uint32_t));

    memset(command, 0, CMD_SIZE + 1);
    while (true) {
        if(xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            memset(dtmp, 0, 4);
            switch(event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_NUM, dtmp, event.size, portMAX_DELAY);
                    if (event.size == 3) {
                        move_arrow(&cursor, command, *((uint32_t*)dtmp));
                        continue;
                    }
                    else if (event.size == 1) {
                        if (*dtmp == '\r') { // user press Enter
                            cursor = 0;
                            enter_button(command);
                        }
                        else if (*dtmp == 127) // user press Backspace
                            backspace(&cursor, command);
                        else
                            printchar(&cursor, command, dtmp, &event);
                    }
                    else 
                        ESP_LOGI(TAG, "[DATA EVT]: %s, %d, cur = %d, size = %d",
                                dtmp, *dtmp, cursor, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
}

void init_uart_console(uart_config_t *uart_config, uint8_t console_priority) {
    esp_log_level_set(TAG, ESP_LOG_NONE);
    uart_driver_install(UART_NUM, BUF_SIZE, BUF_SIZE, 5, &uart0_queue, 0);
    uart_param_config(UART_NUM, uart_config);
    uart_set_pin(UART_NUM, UART_TX, UART_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_pattern_queue_reset(UART_NUM, 5);
    xTaskCreate(console_task, "console_task", 4048, NULL, console_priority, NULL);
}

void clear_cmd_list() {
    t_list *p = NULL;

    if (!cmd_list)
        return;
    while (cmd_list) {
        if (cmd_list->next == NULL) {
            free(cmd_list);
            cmd_list = NULL;
        }
        else {
            p = cmd_list->next;
            free(cmd_list->data);
            free(cmd_list);
            cmd_list = p;
        }
    }
}

void add_cmd(char *cmd_name, void *(*cmd_handler)(void *argc)) {
    t_console_cmd *cmd = (t_console_cmd*)malloc(sizeof(t_console_cmd));

    cmd->cmd_name = cmd_name;
    cmd->cmd_handler = cmd_handler ;
    mx_push_front(&cmd_list, cmd);
}

void uart_printstr(char *str) {
    if (str)
        uart_write_bytes(UART_NUM, str, strlen(str));
}

void uart_print_nl() {
    uart_write_bytes(UART_NUM, NL_SEQ, 2);
}
