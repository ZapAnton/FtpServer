#include "commands/type.h"

void run_type(struct user* const current_user, char* argument) {
    char* type = NULL;
    if (strcmp(argument, "I") == 0) {
        type = "IMAGE";
    } else if (strcmp(argument, "A") == 0) {
        type = "ASCII";
    }
    char response[BUFFER_SIZE] = {'\0'};
    sprintf(response, "200 Setting %s type.\r\n", type);
    send_response(current_user->control_socket, response);
}
