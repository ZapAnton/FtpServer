#include "commands/pwd.h"

void run_pwd(const struct user* const current_user) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char response[BUFFER_SIZE];
	const size_t current_directory_length = sizeof(current_user->current_directory) + sizeof(char);
    const int bytes_written = snprintf(response, current_directory_length, "257 %s\r\n", current_user->current_directory);
    if ((bytes_written < 0) || ((unsigned int) bytes_written >= current_directory_length)) {
        return;
    }
    send_response(current_user->control_socket, response);
}
