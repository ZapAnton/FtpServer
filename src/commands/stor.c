#include "commands/stor.h"

void run_stor(struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection for entry list\r\n");
    const int client_socket = (current_user->data_connection_type == ACTIVE) ? current_user->data_socket : current_user->client_data_socket;
    const size_t filepath_length = strlen(current_user->current_directory) + 1 + strlen(argument);
    char* path = calloc(filepath_length + 1, sizeof(char));
    snprintf(path, filepath_length + 1, "%s/%s", current_user->current_directory, argument);
    save_file(current_user, client_socket, path);
    free(path);
    close(client_socket);
}
