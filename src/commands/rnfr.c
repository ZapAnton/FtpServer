#include "commands/rnfr.h"

void run_rnfr(struct user* const current_user, const char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {}
	
    /*char filename[BUFFER_SIZE];
    snprintf(filename, BUFFER_SIZE, "%s/%s", current_user->current_directory, argument);
    if (access(filename, F_OK) == -1) {
        send_response(current_user->control_socket, "550 File or directory not found.");
        return;
    }
    strncpy(current_user->rnfr_name, argument, BUFFER_SIZE);
    send_response(current_user->control_socket, "350 RNFR accepted - file exists, ready for destination.");
    */
}


