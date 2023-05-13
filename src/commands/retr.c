#include "commands/retr.h"

void run_retr(struct user* current_user, const char* const filepath, const struct Config* config) {
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
	
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    const char* bz2_extension = ".bz2";
    enum CompressorType compressor_type = GZIP;
    char* path = NULL;
    if (!str_ends_with(filepath, bz2_extension)) {
        const size_t filepath_length = strlen(config->server_directory) + 1 + strlen(filepath);
        path = calloc(filepath_length + 1, sizeof(char));
        snprintf(path, filepath_length + 1, "%s/%s", config->server_directory, filepath);
    } else {
        compressor_type = BZIP2;
        const size_t filename_length = strlen(filepath) - strlen(bz2_extension);
        char* filename = calloc(filename_length + 1, sizeof(char));
        strncpy(filename, filepath, filename_length);
        const size_t filepath_length = strlen(config->server_directory) + 1 + filename_length;
        path = calloc(filepath_length + 1, sizeof(char));
        snprintf(path, filepath_length + 1, "%s/%s", config->server_directory, filename);
        free(filename);
    }
    const int client_socket = (current_user->data_connection_type == ACTIVE) ? current_user->data_socket : current_user->client_data_socket;
    if (!file_exists(path)) {
        send_response(current_user->control_socket, "550 File unavailable.\r\n");
        close(client_socket); 
        free(path);
        return;
    }
    if (is_dir(path)) {
        transfer_dir(current_user, client_socket, path, compressor_type, config);
    } else {
        transfer_file(current_user, client_socket, path);
    }
    free(path);
    // Закрытие соединения для передачи данных
    close(client_socket);
}
