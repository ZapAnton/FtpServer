#include "commands/pasv.h"

void run_pasv(struct user* const current_user) {
    // Получение адреса сервера и порта, на котором будет прослушиваться соединение для передачи данных
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = 0;
    // Создание сокета для передачи данных и привязка его к заданному адресу сервера
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Установка сокета на переиспользование порта
    const int option = 1;
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (bind(data_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
		fprintf(stderr, "Data connection socket binding error: %s\n", strerror(errno));
        send_response(current_user->control_socket, "425 Can't open data connection.\r\n");
        return;
    }
    // Перевод сокета в режим прослушивания
    listen(data_socket, 1);
    // Получение порта, на котором был создан сокет
    socklen_t server_address_length = sizeof(server_address);
    getsockname(data_socket, (struct sockaddr*)&server_address, &server_address_length);
    unsigned int data_port = ntohs(server_address.sin_port);
    // Сохранение адреса и порта для передачи данных в структуре current_user
    current_user->data_socket = data_socket;
    memcpy(&(current_user->data_address), &server_address, sizeof(server_address));
    // Отправка клиенту сообщения с IP-адресом и портом для соединения
    char response[BUFFER_SIZE];
    const char* ip_addr = inet_ntoa(server_address.sin_addr);
    char ip_addr_ftp[16] = {'\0'};
    strcpy(ip_addr_ftp, ip_addr);
    for (size_t i = 0; i < strlen(ip_addr_ftp); i += 1) {
        if (ip_addr_ftp[i] == '.') {
            ip_addr_ftp[i] = ',';
        }
    }
    snprintf(response, BUFFER_SIZE, "227 Entering Passive Mode (%s,%d,%d)\r\n", ip_addr_ftp, data_port / 256, data_port % 256);
    send_response(current_user->control_socket, response);
    current_user->data_connection_type = PASSIVE;
}
