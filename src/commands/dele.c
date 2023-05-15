#include "commands/dele.h"

void run_dele(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }

    char filepath[BUFFER_SIZE] = {0};
    get_absolute_path(argument, filepath, current_user->current_directory);
    if (remove(filepath) == 0) {
        send_response(current_user->control_socket, "250 File deleted.\r\n");
    } else {
        send_response(current_user->control_socket, "550 Failed to delete file.\r\n");
    }
}
