#include "commands/rnto.h"

void run_rnto(struct user* const current_user, const char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    if (!argument) {
		send_response(current_user->control_socket, "550 Invalid new file name or new directory name\r\n");
        return;
	}
	
    char new_filename[BUFFER_SIZE] = { '\0' };
    sprintf(new_filename, "%s/%s", current_user->current_directory, argument);
	//pthread_mutex_lock(&fs_mutex);
    if (rename_mutex(current_user->rnfr_name, new_filename) == -1) {
        send_response(current_user->control_socket, "550 Failed to rename file or directory.\r\n");
        return;
    }
	
	//pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, "250 RNTO successful.\r\n");
	memset(current_user->rnfr_name, '\0', strlen(current_user->rnfr_name));
}
