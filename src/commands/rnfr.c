#include "commands/rnfr.h"

void run_rnfr(struct user* const current_user, const char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }

    if (!argument) {
        send_response(current_user->control_socket, "550 Invalid file name or directory name\r\n");
        return;
    }

    char filename[BUFFER_SIZE] = {'\0'};
    sprintf(filename, "%s/%s", current_user->current_directory, argument);
    if (!file_exists(filename)) {
        send_response(current_user->control_socket, "550 File or directory not found.\r\n");
        return;
    }
    strcpy(current_user->rnfr_name, filename);
    send_response(current_user->control_socket,
                  "350 RNFR accepted - file exists, ready for destination.\r\n");
}
