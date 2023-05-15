#include "commands/pass.h"

void run_pass(struct user* const current_user, const char* const argument,
              const struct Config* config) {
    if (strcmp(current_user->name, config->username) == 0 &&
        strcmp(argument, config->password) == 0) {
        current_user->authenticated = true;
        send_response(current_user->control_socket, "230 Logged in.\r\n");
    } else {
        send_response(current_user->control_socket, "530 Login incorrect.\r\n");
    }
}
