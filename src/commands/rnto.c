#include "commands/rnto.h"

void run_rnto(struct user* const current_user, const char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {}
	
    /*
    char old_filename[BUFFER_SIZE] = { 0 };
    char new_filename[BUFFER_SIZE] = { 0 };
    snprintf(old_filename, BUFFER_SIZE + 1, "%s/%s", current_user->current_directory, current_user->rnfr_name);
    snprintf(new_filename, BUFFER_SIZE + 1, "%s/%s", current_user->current_directory, argument);
    if (rename(old_filename, new_filename) == -1) {
        send_response(current_user->control_socket, "550 Failed to rename file or directory.");
        return;
    }
    send_response(current_user->control_socket, "250 RNTO successful.");
    */
	//TODO: current_user->rnfr_name нужно ли очищать?
}
