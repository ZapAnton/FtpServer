#include "commands/abor.h"

void run_abor(struct user* const current_user) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    if (current_user->data_socket > 0) {
        close(current_user->data_socket);
        current_user->data_socket = -1;
    }
    current_user->is_aborted = 1;
    send_response(current_user->control_socket, "226 Abort successful.\r\n");
}
