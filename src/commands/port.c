#include "commands/port.h"

void run_port(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    // Разбиение аргумента на IP-адрес и порт
    char* ip1 = strtok(argument, ",");
    char* ip2 = strtok(NULL, ",");
    char* ip3 = strtok(NULL, ",");
    char* ip4 = strtok(NULL, ",");
    int port1 = atoi(strtok(NULL, ","));
    int port2 = atoi(strtok(NULL, ","));
    char ip_str[16] = {'\0'};
    snprintf(ip_str, 16, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);
    int data_port = port1 * 256 + port2;
    // Преобразование IP-адреса в структуру sockaddr_in
    struct sockaddr_in data_address = {0};
    memset(&data_address, 0, sizeof(data_address));
    inet_pton(AF_INET, ip_str, &data_address.sin_addr);
    data_address.sin_family = AF_INET;
    data_address.sin_port = htons(data_port);
    // Сохранение IP-адреса и порта для передачи данных в структуре current_user
    memcpy(&(current_user->data_address), &data_address, sizeof(data_address));
    current_user->data_connection_type = ACTIVE;
    current_user->is_aborted = false;
    // Отправка клиенту сообщения об успешной установке порта для передачи данных
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "200 PORT command successful (%s:%d).\r\n", ip_str, data_port);
    send_response(current_user->control_socket, response);
}
