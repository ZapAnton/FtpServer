#include "commands/mkd.h"

void run_mkd(const struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char response[BUFFER_SIZE];
    char directory[BUFFER_SIZE];
    get_absolute_path(argument, directory, current_user->current_directory);
    if (mkdir(directory, 0777) == 0) {
        snprintf(response, BUFFER_SIZE, "257 \"%s\" created\r\n", argument);
    } else {
        snprintf(response, BUFFER_SIZE, "550 %s\r\n", strerror(errno));
    }
    send_response(current_user->control_socket, response);
}
