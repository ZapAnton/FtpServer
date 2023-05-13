#include "commands/user.h"

void run_user(struct user* const current_user, const char* const argument, const struct Config* config) {
    if (strcmp(argument, config->username) == 0) {
        send_response(current_user->control_socket, "331 Please specify the password.\r\n");
        strncpy(current_user->name, argument, MAX_USERNAME_LENGTH - 1);
    } else {
        send_response(current_user->control_socket, "332 Need account for login.\r\n");
    }
}
