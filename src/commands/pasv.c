#include "commands/pasv.h"

void run_pasv(struct user* const current_user, const char* const argument) {
	if (!current_user || !argument) {}
}

/*void process_pasv_command(int client_socket, struct user* current_user) {
    // Получение адреса сервера и порта, на котором будет прослушиваться соединение для передачи данных
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = 0;

    // Создание сокета для передачи данных и привязка его к заданному адресу сервера
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    bind(data_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Перевод сокета в режим прослушивания
    listen(data_socket, 1);

    // Получение порта, на котором был создан сокет
    socklen_t server_address_length = sizeof(server_address);
    getsockname(data_socket, (struct sockaddr*)&server_address, &server_address_length);
    int data_port = ntohs(server_address.sin_port);

    // Сохранение адреса и порта для передачи данных в структуре current_user
    memcpy(&(current_user->data_address), &server_address, sizeof(server_address));

    // Отправка клиенту сообщения с IP-адресом и портом для соединения
    char response[BUFFER_SIZE];
    unsigned char* p = (unsigned char*)&server_address.sin_addr;
    snprintf(response, BUFFER_SIZE, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",
             p[0], p[1], p[2], p[3], data_port / 256, data_port % 256);
    send_response(client_socket, response);
}*/
