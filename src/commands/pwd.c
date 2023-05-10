#include "commands/pwd.h"

void run_pwd(const struct user* const current_user) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char* current_directory = current_user->current_directory;
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "257 \"%s\"\r\n", current_directory);
    send_response(current_user->control_socket, response);
}
