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
	
    char response[BUFFER_SIZE];
    char directory[PARAM_SIZE];
	const size_t response_length = BUFFER_SIZE;
    get_absolute_path(argument, directory, current_user->current_directory);
	pthread_mutex_lock(&fs_mutex);
    if (mkdir(directory, 0777) == 0) {
        snprintf(response, response_length, "257 \"%s\" created\r\n", argument);
    } else {
        snprintf(response, response_length, "550 %s\r\n", strerror(errno));
    }
	
	pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, response);
}