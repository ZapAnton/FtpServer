#include "commands/cwd.h"

void run_cwd(struct user* const current_user, char* const argument, const struct Config* const config) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }

    char new_directory[PARAM_SIZE] = { '\0' };
    if (argument == NULL) {
        strcpy(new_directory, config->server_directory);
    } else if (strcmp(argument, "..") == 0) {
        // Обработка пути на уровень выше
        char* last_slash = strrchr(current_user->current_directory, '/');
        if (last_slash == NULL || last_slash == current_user->current_directory) {
            // Если текущая директория - корневая, то новой директорией станет корень сервера
            strcpy(new_directory, config->server_directory);
        } else {
            // Иначе - директория на уровень выше
            *last_slash = '\0';
            strcpy(new_directory, current_user->current_directory);
        }
    } else if (argument[0] == '/') {
        // Полный путь		
        strcpy(new_directory, argument);	
    } else {
        // Относительный путь
		const size_t new_directory_length = strlen(current_user->current_directory) + strlen(argument) + 4;
        snprintf(new_directory, new_directory_length, "%s/%s", current_user->current_directory, argument);
    }

    // Проверка, что новый путь существует
    if (!file_exists(new_directory)) {
        send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
    }
	
	if (strcmp(new_directory, config->server_directory) != 0 && strstr(new_directory, config->server_directory) == NULL) {
		send_response(current_user->control_socket, "550 Permission denied.\r\n");
		return;
	}
		
    strcpy(current_user->current_directory, new_directory);
    char response[BUFFER_SIZE];
    sprintf(response, "250 Directory changed to %s\r\n", new_directory);
    send_response(current_user->control_socket, response);
}