#include "commands/cwd.h"

void run_cwd(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
	char* new_directory = argument;
    // Обработка относительного пути
    if (argument[0] != '/') {
        char* current_directory = current_user->current_directory;
        size_t len = strlen(current_directory) + strlen(argument) + 2;
        new_directory = (char*) malloc(len);
        snprintf(new_directory, len, "%s/%s", current_directory, argument);
    }
    // Проверка, что новый путь существует
    if (access(new_directory, F_OK) == -1) {
        send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
    }
    // Обновление текущего рабочего каталога
    free(current_user->current_directory);
    current_user->current_directory = new_directory;
    send_response(current_user->control_socket, "250 Directory changed\r\n");
}
