#include "commands/stor.h"

void run_stor(struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    if (current_user->data_socket == -1) {
        send_response(current_user->control_socket, "425 Use PORT or PASV first.");
        return;
    }
    char path[BUFFER_SIZE];
	get_absolute_path(argument, path, current_user->current_directory);
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        send_response(current_user->control_socket, "550 Failed to open file.");
        return;
    }
    int n = 0;
    char buf[BUFFER_SIZE] = { 0 };
    while ((n = read(current_user->data_socket, buf, BUFFER_SIZE)) > 0) {
        if (fwrite(buf, 1, n, file) != (size_t) n) {
            send_response(current_user->control_socket, "550 Failed to write file.");
            fclose(file);
            return;
        }
    }
    fclose(file);
    send_response(current_user->control_socket, "226 Transfer complete.");
}
