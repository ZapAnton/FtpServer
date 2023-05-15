#include "commands/cwd.h"

void run_cwd(struct user* const current_user, char* const argument,
             const struct Config* const config) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    if (argument == NULL) {
        strcpy(current_user->current_directory, config->server_directory);
        goto send_cwd_message;
    } else if (strcmp(argument, "..") == 0) {
        char* last_file_separator = strrchr(current_user->current_directory, '/');
        if (last_file_separator != NULL) {
            *last_file_separator = '\0';
        }
        goto send_cwd_message;
    } else if (strstr(argument, "..") != NULL) {
        send_response(current_user->control_socket, "550 Specified path is not permitted.\r\n");
        return;
    }
    char new_directory[BUFFER_SIZE] = {'\0'};
    sprintf(new_directory, "%s/%s", current_user->current_directory, argument);
    if (!file_exists(new_directory) || !is_dir(new_directory)) {
        send_response(current_user->control_socket, "550 Specified path does not exist.\r\n");
        return;
    }
    strcpy(current_user->current_directory, new_directory);
send_cwd_message:;
    char response[BUFFER_SIZE] = {'\0'};
    sprintf(response, "250 Directory changed to %s\r\n", current_user->current_directory);
    send_response(current_user->control_socket, response);
}
