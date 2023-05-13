#include "commands/list.h"

void run_list(struct user* const current_user, const char* const argument, const struct Config* config) {    
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    int data_socket = 0;
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection for entry list\r\n");

    // Получение списка файлов
    size_t directory_path_length = strlen(config->server_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s", config->server_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", config->server_directory);
    }
    DIR* directory = opendir(directory_path);
	if (directory == NULL) {
		send_response(current_user->control_socket, "550 Failed to open directory.");
		return;
	}
    struct dirent* entry;
    char buffer[BUFFER_SIZE];
    while ((entry = readdir(directory)) != NULL) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		sprintf(buffer, "%s/%s", directory_path, entry->d_name);
		struct stat info;
		if (stat(buffer, &info) == -1) {
			send_response(current_user->control_socket, "550 Failed to get file information.");
			closedir(directory);
			return;
		}
        /* TODO: Rewrite with snprintf
		char *type = (S_ISDIR(info.st_mode)) ? "d" : "-";
		char *perms = format_perms(info.st_mode);
		char *mtime = format_time(info.st_mtime);
		char filename[BUFFER_SIZE];
		strncpy(filename, entry->d_name, BUFFER_SIZE);
		send_response(data_socket, "%s%s %3lu %-8d %-8d %8lu %s %s\r\n", type, perms,
					  (unsigned long)info.st_nlink, info.st_uid, info.st_gid,
					  (unsigned long)info.st_size, mtime, filename);
		free(perms);
		free(mtime);
        */
	}
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(data_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}
