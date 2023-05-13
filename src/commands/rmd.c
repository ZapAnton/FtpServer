#include "commands/rmd.h"

void run_rmd(const struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (!argument) {}
	/*
    char absolute_path[BUFFER_SIZE];
    get_absolute_path(argument, absolute_path, current_user->current_directory);

    // Проверка, существует ли каталог и есть ли у пользователя разрешение на его удаление
    if (access(absolute_path, F_OK) == -1 || access(absolute_path, W_OK) == -1) {
        send_response(current_user->data_socket, "550 Requested action not taken. File unavailable.");
        return;
    }

    // Попытка удалить каталог
    if (rmdir(absolute_path) == -1) {
        send_response(current_user->data_socket, "550 Requested action not taken. File unavailable.");
        return;
    }
	
    send_response(current_user->data_socket, "250 Requested file action okay, completed.");
    */
}


