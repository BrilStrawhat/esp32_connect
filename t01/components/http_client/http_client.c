#include "http_client.h"

static const char *TAG = "HTTP_CLIENT";

static void error(const char *msg) { ESP_LOGE(TAG, "%s", msg); }

static uint8_t http_client(char *host, uint32_t port, char *message) {
    // int portno =        5000;
    // char *host =        "192.168.0.102";
    // char *message = "POST / HTTP/1.0\r\n\r\n";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;
    char response[4096];

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
        return 1;
    }

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) {
        error("ERROR, no such host");
         return 1;
    }

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
        return 1;
    }

    /* send the request */
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sockfd, message+sent, total-sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
            return 1;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    /* receive the response */
    memset(response, 0, sizeof(response));
    total = sizeof(response) - 1;
    received = 0;
    do {
        bytes = read(sockfd, response + received, total - received);
        if (bytes < 0) {
            error("ERROR reading response from socket");
            return 1;
        }
        if (bytes == 0)
            break;
        received += bytes;
    } while (received < total);

    if (received == total) {
        error("ERROR storing complete response from socket");
        return 1;
    }

    /* close the socket */
    close(sockfd);

    /* process response */
    printf("Response:\n%s\n",response);
    return 0;
}


static void echo_cmd(char *host, uint32_t port, char *message, uint8_t repeat) {
    uint8_t resp = 0;
    char str[32] = {0};

    for(uint8_t i = 0; i < repeat; i++) {
        resp = http_client(host, port, "POST / HTTP/1.0\r\n\r\n");
        uart_print_nl();
        uart_printstr("PING #");

        if (resp == ESP_OK) {
            sprintf(str, "%d SUCCESS", i);
            uart_printstr(str);
        }
        else {
            sprintf(str, "%d FAILED", i);
            uart_printstr(str);
        }
    }
}

void *http(void *arg) {
    if (arg == NULL || *(char*)arg == '\0')
        return NULL;
    char *saveptr = NULL;
    char delim = ' ';
    char *method = strtok_r((char*)arg, &delim, &saveptr);

    // if (!strcmp(method, "kk")) {
        // echo_cmd("192.168.0.102", 5000, "POST / HTTP/1.0\r\n\r\n", 3);
        // return NULL;
    // }
    if (method && !strcmp(method, "post")) {
        char *cmd = strtok_r(NULL, &delim, &saveptr);

        if (cmd && !strcmp(cmd, "echo")) {
            char *host = strtok_r(NULL, &delim, &saveptr); 
            char *port = strtok_r(NULL, &delim, &saveptr); 
            char *repeat = strtok_r(NULL, &delim, &saveptr); 

            if (host && port && repeat) {
                echo_cmd(host, atoi(port), "POST / HTTP/1.0\r\n\r\n", 3);
            }
        }

    }
    else {
        uart_print_nl();
        uart_printstr("http usage: http method command args");
    }
    return NULL;
}
