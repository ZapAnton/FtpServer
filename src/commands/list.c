#include "commands/list.h"

void run_list(struct user* const current_user, const char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (establish_data_connection(current_user) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket,
                  "150 Opening data connection\r\n");
    // Получение списка файлов
    size_t directory_path_length = strlen(current_user->current_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s",
                 current_user->current_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", current_user->current_directory);
    }
    DIR* directory = opendir(directory_path);
    if (directory == NULL) {
        send_response(current_user->control_socket, "550 Failed to open directory\r\n.");
        return;
    }
    const int client_socket = (current_user->data_connection_type == ACTIVE)
                                  ? current_user->data_socket
                                  : current_user->client_data_socket;
    struct dirent* entry = NULL;
    char buffer[BUFFER_SIZE] = {0};
    while ((entry = readdir(directory)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        sprintf(buffer, "%s/%s", directory_path, entry->d_name);
        struct stat info = {0};
        if (stat(buffer, &info) == -1) {
            send_response(current_user->control_socket, "550 Failed to get file information.\r\n");
            closedir(directory);
            return;
        }
        char* type = is_dir(buffer) ? "d" : "-";
        char* perms = format_perms(info.st_mode);
        char* mtime = format_time(info.st_mtime);
        char fileinfo[BUFFER_SIZE] = {'\0'};
        sprintf(fileinfo, "%s%s %3lu %-8d %-8d %8lu %s %s\r\n", type, perms,
                (unsigned long)info.st_nlink, info.st_uid, info.st_gid, (unsigned long)info.st_size,
                mtime, entry->d_name);
        send_response(client_socket, fileinfo);
        free(perms);
        free(mtime);
    }
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(client_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}
