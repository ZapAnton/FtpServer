#include "commands/nlst.h"

void run_nlst(struct user* current_user, const char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket,
                  "150 Opening data connection\r\n");

    // Получение списка файлов
    size_t directory_path_length = strlen(current_user->current_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s",
                 current_user->current_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", current_user->current_directory);
    }
    DIR* directory = opendir(directory_path);
    struct dirent* entry = NULL;
    char buffer[BUFFER_SIZE] = { '\0' };
    const int client_socket = (current_user->data_connection_type == ACTIVE)
                                  ? current_user->data_socket
                                  : current_user->client_data_socket;
    while ((entry = readdir(directory)) != NULL) {
        snprintf(buffer, BUFFER_SIZE, "%s\r\n", entry->d_name);
        send(client_socket, buffer, strlen(buffer), 0);
    }
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(client_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}
