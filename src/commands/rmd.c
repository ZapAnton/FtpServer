#include "commands/rmd.h"

void run_rmd(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {
        send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
    }

    char filepath[BUFFER_SIZE];
    sprintf(filepath, "%s/%s", current_user->current_directory, argument);
    if (!file_exists(filepath)) {
        send_response(current_user->control_socket, "550 Specified directory does not exist.\r\n");
        return;
    }
    // Попытка удалить каталог
    if (rmdir_mutex(filepath) != 0) {
        send_response(current_user->control_socket, "550 Failed to delete directory.\r\n");
        return;
    }
    send_response(current_user->control_socket, "250 Requested directory delete okay.\r\n");
}
