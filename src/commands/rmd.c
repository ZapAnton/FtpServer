#include "commands/rmd.h"

void run_rmd(struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {
		send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
	}
	
    char absolute_path[PARAM_SIZE];
    get_absolute_path(argument, absolute_path, current_user->current_directory);

    // Проверка, существует ли каталог и есть ли у пользователя разрешение на его удаление
	//pthread_mutex_lock(&fs_mutex);
    if (access(absolute_path, F_OK) == -1 || access(absolute_path, W_OK) == -1) {
        send_response(current_user->control_socket, "550 Requested action not taken. File unavailable.");
        //pthread_mutex_unlock(&fs_mutex);
		return;
    }

    // Попытка удалить каталог
    if (rmdir(absolute_path) == -1) {
        send_response(current_user->control_socket, "550 Requested action not taken. File unavailable.");
        //pthread_mutex_unlock(&fs_mutex);
		return;
    }
	
	//pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, "250 Requested file action okay, completed.");
}


