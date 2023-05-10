#include "commands/retr.h"

void run_retr(struct user* current_user, const char* const filepath, const Config* config) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection\r\n");
	//TODO: С этим надо что-то сделать....
	/*if (current_user->data_socket < 0) {
        send_response(current_user->control_socket, "425 No data connection");
        return;
    }
    if (current_user->is_aborted) {
        send_response(current_user->control_socket, "426 Connection aborted");
        return;
    }*/
	
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    size_t filepath_length = strlen(config->server_directory) + 1 + strlen(filepath);
    char* path = calloc(filepath_length + 1, sizeof(char));
    snprintf(path, filepath_length + 1, "%s/%s", config->server_directory, filepath);
    if (is_dir(path)) {
        transfer_dir(current_user, data_socket, path, config);
    } else {
        transfer_file(current_user, data_socket, path);
    }
    free(path);
    // Закрытие соединения для передачи данных
    close(data_socket);
}
