#include "commands/mkd.h"

void run_mkd(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {
        send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
    }
    char response[BUFFER_SIZE] = {'\0'};
    char directory[BUFFER_SIZE] = {'\0'};
    sprintf(directory, "%s/%s", current_user->current_directory, argument);
    if (mkdir_mutex(directory, 0777) == 0) {
        sprintf(response, "257 \"%s\" created\r\n", argument);
    } else {
        sprintf(response, "550 Failed to create directory: %s\r\n", strerror(errno));
    }
    send_response(current_user->control_socket, response);
}
