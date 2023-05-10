#include "commands/nlst.h"

void run_nlst(struct user* current_user, const char* const argument, const Config* config) {    
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection for entry list\r\n");

    // Получение списка файлов
    size_t directory_path_length = strlen(config->server_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s", config->server_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", config->server_directory);
    }
    DIR* directory = opendir(directory_path);
    struct dirent* entry;
    char buffer[BUFFER_SIZE];
    while ((entry = readdir(directory)) != NULL) {
        snprintf(buffer, BUFFER_SIZE, "%s\r\n", entry->d_name);
        send(data_socket, buffer, strlen(buffer), 0);
    }
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(data_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}
