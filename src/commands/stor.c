#include "commands/stor.h"

void run_stor(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    send_response(current_user->control_socket,
                  "150 Opening data connection\r\n");
    enum CompressorType compressor_type = NONE;
    if (str_ends_with(argument, ".tar.gz")) {
        compressor_type = GZIP;
    } else if (str_ends_with(argument, ".bz2")) {
        compressor_type = BZIP2;
    }
    const int client_socket = (current_user->data_connection_type == ACTIVE)
                                  ? current_user->data_socket
                                  : current_user->client_data_socket;
    const size_t filepath_length = strlen(current_user->current_directory) + 1 + strlen(argument);
    char* path = calloc(filepath_length + 1, sizeof(char));
    snprintf(path, filepath_length + 1, "%s/%s", current_user->current_directory, argument);
    save_file(current_user, client_socket, path);
    if (compressor_type != NONE) {
        make_archive_operation(current_user->current_directory, path, compressor_type, EXTRACT);
    }
    free(path);
    close(client_socket);
}
