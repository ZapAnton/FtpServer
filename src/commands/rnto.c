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
	
    char old_filename[PARAM_SIZE] = { '\0' };
    char new_filename[PARAM_SIZE] = { '\0' };
	const size_t filename_length = PARAM_SIZE;
    snprintf(old_filename, filename_length + sizeof(char), "%s/%s", current_user->current_directory, current_user->rnfr_name);
    snprintf(new_filename, filename_length + sizeof(char), "%s/%s", current_user->current_directory, argument);
	//pthread_mutex_lock(&fs_mutex);
    if (rename(old_filename, new_filename) == -1) {
        send_response(current_user->control_socket, "550 Failed to rename file or directory.\r\n");
		//pthread_mutex_unlock(&fs_mutex);
        return;
    }
	
	//pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, "250 RNTO successful.\r\n");
	
	//TODO: current_user->rnfr_name нужно ли очищать?
}
